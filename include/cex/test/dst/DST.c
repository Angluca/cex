#pragma once
#include "DST.h"
#include "cex/cex.h"
#include <stdatomic.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

DST_c _DST = { 0 };

Exception
DST_create(u64 seed)
{
    e$assert(seed > 0);
    e$assert(atomic_is_lock_free(&_DST._lock) && "Atomic is not lock free/not supported!");
    e$assertf(
        !atomic_flag_test_and_set(&_DST._lock),
        "Already in use OR access from different thread OR DST.create() called before DST.destroy()"
    );
    e$assert(
        !atomic_flag_test_and_set(&_DST._lock_rng) &&
        "Already in use or access from different thread or uninitialized DSTState"
    );
    e$assert(_DST._initial_seed == 0 && "expected uninitialized");

    memset(&_DST, 0, sizeof(_DST));

    Random.seed(&_DST._rng, _DST._initial_seed);
    _DST._initial_seed = seed;
    _DST._initial_time = time(NULL);

    atomic_flag_test_and_set(&_DST._lock);
    atomic_flag_clear(&_DST._lock_rng);

    return EOK;
}

Exception
DST_setup(dst_params_s* prm)
{
    e$assert(_DST._initial_seed > 0);
    e$assertf(
        _DST._tick_current == 0 || _DST._tick_current >= _DST._tick_max,
        "Trying to run DST.setup() in unfinished or after stopped DST.tick() loop?"
    );

    if (prm != NULL) {
        e$assert(prm->allocators.malloc_fail_prob >= 0 && prm->allocators.malloc_fail_prob <= 1);
        memcpy(&_DST.prm, prm, sizeof(_DST.prm));
    } else {
        memset(&_DST.prm, 0, sizeof(_DST.prm));
    }

    _DST._tick_max = 0;
    _DST._tick_current = 0;

    return EOK;
}

bool
DST_rnd_check(f32 prob_threshold)
{
    // DST is designed to be used only single threaded! Assert this.
    uassert(!atomic_flag_test_and_set(&_DST._lock_rng) && "already locked, using multi-threading?");

    bool result = Random.prob(&_DST._rng, prob_threshold);

    atomic_flag_clear(&_DST._lock_rng);
    return result;
}

bool
DST_tick(u32 max_ticks)
{
    uassert(max_ticks > 0);

    if (_DST._tick_max == 0) {
        _DST._tick_max = max_ticks;
    }
    uassert(
        _DST._tick_max == max_ticks && "max_ticks mismatch, maybe running twice without DST.setup()"
    );

    uassert(
        _DST._tick_current <= _DST._tick_max &&
        "tick overflow, maybe running twice without DST.setup()"
    );

    // Adding extra random step
    Random.next(&_DST._rng);


    bool result = _DST._tick_current++ < _DST._tick_max;
    if (result) {
        // Still working
        if (_DST._sim_log != NULL && !_DST.prm.simulator.keep_tick_log) {
            DST.sim_log_reset();
        }
    }
    return result;
}

void
DST_sim_log_reset()
{
    uassert(_DST._sim_log != NULL && "expected to be called in simulation");
    fseek(_DST._sim_log, 0, SEEK_SET);
    fprintf(_DST._sim_log, "Sim log tick#: %lu\n", _DST._tick_current);
}

Exception
DST_simulate(DSTSimulationFunc_f simfunc)
{
    Exc result = Error.runtime;
    e$assert(simfunc != NULL);
    uassert(_DST._sim_log == NULL && "recursive simulation calls forbidden");

    // NOTE: randomising rng, otherwise all forked random will be the same in fuzzf()
    Random.next(&_DST._rng);

    // we store all logging output from the simulation run in the shared memory
    const usize mmap_size = 1024 * 1024 * 10;
    char* log_buf = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (log_buf == MAP_FAILED) {
        goto end;
    }

    e$except_null(_DST._sim_log = fmemopen(log_buf, mmap_size, "r+w"))
    {
        goto end;
    }
    e$except_errno(setvbuf(_DST._sim_log, NULL, _IONBF, 0))
    {
        goto end;
    }

    // WARNING : everything after this line may be child process (FORK)
    pid_t childPid = fork();
    if (childPid == 0) // fork succeeded
    {
        uassert(_DST._sim_log != NULL);
        stderr = _DST._sim_log;
        stdout = _DST._sim_log;

        // WE ARE IN CHILD PROCESS HERE
        e$except_silent(err, simfunc())
        {
            exit(1); // exits child
        }

        // succeeded, exits child only
        exit(0);
    } else if (childPid < 0) {
        // WE ARE IN PARENT PROCESS HERE
        log$error("fork() failed");
        goto end;
    } else {
        // WE ARE IN PARENT PROCESS HERE
        int returnStatus;
        waitpid(childPid, &returnStatus, 0); // Parent process waits here for child to terminate.

        if (returnStatus == 0) // Verify child process terminated without error.
        {
            result = EOK;
        } else {

            // Printing log contents on failure
            printf("========  SIM FAILED LOG ===========\n");
            fflush(stdout);

            e$except_errno(fseek(_DST._sim_log, 0, SEEK_SET))
            {
                ;
            }

            char ch = 0;
            usize pos = 0;
            while ((ch = fgetc(_DST._sim_log)) != EOF) {
                if (ch == '\0') {
                    break;
                }
                putc(ch, stdout);
                pos++;
            }
            printf("========  POSTMORTEM ===========\n");
            DST.print_postmortem(NULL);
            printf("========  POSTMORTEM END ===========\n");
            fflush(stdout);

            if (pos >= mmap_size) {
                log$warn(
                    "Possible simulation log buffer truncation, log size > %0.1fM",
                    (double)(mmap_size / (1024 * 1024))
                );
            }

            result = Error.assert;
        }
    }

end:
    if (_DST._sim_log != NULL) {
        fclose(_DST._sim_log);
        _DST._sim_log = NULL;
    }
    if (log_buf != NULL) {
        munmap(log_buf, mmap_size);
    }
    return result;
}

f32
DST_rnd_prob()
{
    // DST is designed to be used only single threaded! Assert this.
    uassert(!atomic_flag_test_and_set(&_DST._lock_rng) && "already locked, using multi-threading?");

    f32 result = Random.f32(&_DST._rng);

    atomic_flag_clear(&_DST._lock_rng);
    return result > 0.05f ? result : 0.05f;
}

usize
DST_rnd_range(usize min, usize max)
{
    // DST is designed to be used only single threaded! Assert this.
    uassert(!atomic_flag_test_and_set(&_DST._lock_rng) && "already locked, using multi-threading?");

    usize result = Random.range(&_DST._rng, min, max);

    atomic_flag_clear(&_DST._lock_rng);
    return result;
}

/**
 * @brief Compatible function with test$set_postmortem(DST.print_postmortem, NULL)
 *
 * @param ctx
 */
void
DST_print_postmortem(void* ctx)
{
    (void)ctx;

    // WARNING: don't use uassert() here, because this will cause infinite recursion on failure!
    printf("DST:\n");
    printf("\tseed: %lu\n", _DST._initial_seed);
    printf("\ttime elapsed: %lu sec\n", time(NULL) - _DST._initial_time);
    printf("\tRandom state:\n");
    printf("\t\tstate[0]: %lu\n", _DST._rng.state[0]);
    printf("\t\tstate[1]: %lu\n", _DST._rng.state[1]);
    printf("\t\tn_ticks: %lu\n", _DST._rng.n_ticks);
}

void
DST_destroy()
{
    _DST._initial_seed = 0;
    atomic_flag_clear(&_DST._lock_rng);
    atomic_flag_clear(&_DST._lock);
}

const struct __class__DST DST = {
    // Autogenerated by CEX
    // clang-format off
    .create = DST_create,
    .setup = DST_setup,
    .rnd_check = DST_rnd_check,
    .tick = DST_tick,
    .sim_log_reset = DST_sim_log_reset,
    .simulate = DST_simulate,
    .rnd_prob = DST_rnd_prob,
    .rnd_range = DST_rnd_range,
    .print_postmortem = DST_print_postmortem,
    .destroy = DST_destroy,
    // clang-format on
};
