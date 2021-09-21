/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, New York University and Max Planck Gesellschaft.
 * All rights reserved.
 */

#pragma once

#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace master_board_sdk
{
namespace benchmark
{
/**
 * @brief This class is a data structure allowing the user to share
 * configurations among threads. These parameter allows you to generate
 * real threads in xenomai and rt_preempt. The same code is compatible with
 * Mac and ubuntu but will run non-real time threads.
 *
 * warning : initial version, copy pasted from :
 * https://wiki.linuxfoundation.org/realtime/documentation/howto/applications/application_base
 * I did not study things now, so this likely needs improvement (alternative:
 * https://rt.wiki.kernel.org/index.php/Threaded_RT-application_with_memory_locking_and_stack_handling_example)
 * note: if failed as mlockall, run executable with sudo or be part of the
 * real_time group or xenomai group.
 */
class RealTimeThreadParameters
{
public:
    /**
     * @brief Construct a new RealTimeThreadParameters object
     */
    RealTimeThreadParameters()
    {
        keyword_ = "real_time_thread";
        priority_ = 80;
        stack_size_ = 50 * PTHREAD_STACK_MIN;
        cpu_id_.clear();
        delay_ns_ = 0;
        block_memory_ = true;
        cpu_dma_latency_ = 0;
    }
    /**
     * @brief Destroy the RealTimeThreadParameters object
     */
    ~RealTimeThreadParameters()
    {
    }

public:
    /** @brief Used in xenomai to define the thread id. */
    std::string keyword_;

    /** @brief Defines the thread priority from 0 to 100. */
    int priority_;

    /** @brief Define the stack size. */
    int stack_size_;

    /** @brief Define the cpu affinity. Which means on which cpu(s) the thread
     * is going to run */
    std::vector<int> cpu_id_;

    /** @brief indicate on which cpu the thread will run (xenomai only) */
    int dedicated_cpu_id_;

    /** @brief @todo Unknow Xenomai parameter  */
    int delay_ns_;

    /** @brief Defines if the thread should block the memory in a "page" or if
     * several pages can be use. Switching memory page is time consumming and
     * a non real time operation. */
    bool block_memory_;

    /** @brief Maximum desired latency of the CPU in microseconds. Set to 0 to
     * get best real-time performance. Set to any negative value if you do not
     * want the thread to change the CPU latency. */
    int cpu_dma_latency_;
};

/**
 * @brief This class allows you to spawn thread. Its parameter are defined
 * above.
 */
class RealTimeThread
{
public:
    /**
     * @brief Construct a new ThreadInfo object
     */
    RealTimeThread()
    {
        std::string rt_preempt_error_message_ =
            "NOTE: This program must be executed with special permission to "
            "get "
            "the required real time permissions.\n"
            "Either use sudo or be part of the \'realtime\' group"
            "Aborting thread creation.";
        thread_.reset(nullptr);
    }

    /**
     * @brief We do not allow copies of this object
     */
    RealTimeThread(const benchmark::RealTimeThread& other) = delete;

    /**
     * @brief Destroy the RealTimeThread object.
     */
    ~RealTimeThread()
    {
        join();
        thread_.reset(nullptr);
        {
            printf("Delete RealTimeThread!\n");
            fflush(stdout);
        }
    }

    /**
     * @brief create_realtime_thread spawns a real time thread if the OS allows
     * it.
     * @param[in] thread_function: the executing function for the thread.
     * @param[in] args: arguments to be passed to the thread.
     * @return the error code.
     */
    int create_realtime_thread(void* (*thread_function)(void*), void* args)
    {
        if (thread_ != nullptr)
        {
            printf("Thread already running");
        }

        if (parameters_.cpu_dma_latency_ >= 0)
        {
            set_cpu_dma_latency(parameters_.cpu_dma_latency_);
        }

        thread_.reset(new pthread_t());

        if (parameters_.block_memory_)
        {
            block_memory();
        }

        struct sched_param param;
        pthread_attr_t attr;
        int ret;

        ret = pthread_attr_init(&attr);
        if (ret)
        {
            printf("%s %d\n",
                   ("init pthread attributes failed. Ret=" +
                    rt_preempt_error_message_)
                       .c_str(),
                   ret);
            return ret;
        }

        /* Set a specific stack size  */
        ret = pthread_attr_setstacksize(&attr, parameters_.stack_size_);
        if (ret)
        {
            printf("%s %d\n",
                   ("pthread setstacksize failed. Ret=" +
                    rt_preempt_error_message_)
                       .c_str(),
                   ret);
            return ret;
        }

        /* Set scheduler policy and priority of pthread */
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        // ret = pthread_attr_setschedpolicy(&attr, SCHED_RR); // WARNING LAAS
        // is using this one!!!!
        if (ret)
        {
            printf("%s %d\n",
                   ("pthread setschedpolicy failed. Ret=" +
                    rt_preempt_error_message_)
                       .c_str(),
                   ret);
            return ret;
        }
        param.sched_priority = parameters_.priority_;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret)
        {
            printf("%s %d\n",
                   ("pthread setschedparam failed. Ret=" +
                    rt_preempt_error_message_)
                       .c_str(),
                   ret);
            return ret;
        }
        /* Use scheduling parameters of attr */
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret)
        {
            printf("%s %d\n",
                   ("pthread setinheritsched failed. Ret=" +
                    rt_preempt_error_message_)
                       .c_str(),
                   ret);
            return ret;
        }

        /* Create a pthread with specified attributes */
        ret = pthread_create(thread_.get(), &attr, thread_function, args);
        if (ret)
        {
            printf("%s %d\n",
                   ("create pthread failed. Ret=" + rt_preempt_error_message_)
                       .c_str(),
                   ret);
            return ret;
        }

        if (parameters_.cpu_id_.size() > 0)
        {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            for (unsigned i = 0; i < parameters_.cpu_id_.size(); ++i)
            {
                CPU_SET(parameters_.cpu_id_[i], &cpuset);
            }
            ret = pthread_setaffinity_np(*thread_, sizeof(cpu_set_t), &cpuset);
            if (ret)
            {
                printf("%s %d\n",
                       ("Associate thread to a specific cpu failed. Ret=" +
                        rt_preempt_error_message_)
                           .c_str(),
                       ret);
            }

            int get_aff_error = 0;
            get_aff_error =
                pthread_getaffinity_np(*thread_, sizeof(cpu_set_t), &cpuset);
            if (get_aff_error)
            {
                printf("%s %d\n",
                       ("Check the thread cpu affinity failed. Ret=" +
                        rt_preempt_error_message_)
                           .c_str(),
                       ret);
            }
            printf("Set returned by pthread_getaffinity_np() contained: ");
            for (unsigned j = 0; j < CPU_SETSIZE; j++)
            {
                if (CPU_ISSET(j, &cpuset))
                {
                    printf("CPU %d, ", j);
                }
            }
            printf("\n");
        }

        std::vector<int> cpu_affinities(1, 0);
        fix_current_process_to_cpu(cpu_affinities, getpid());
        return ret;
    }

    /**
     * @brief join join the real time thread
     * @return the error code.
     */
    int join()
    {
        int ret = 0;
        if (thread_ != nullptr)
        {
            /* Join the thread and wait until it is done */
            ret = pthread_join(*thread_, nullptr);
            if (ret)
            {
                printf("join pthread failed.\n");
            }
            thread_.reset(nullptr);
        }
        return ret;
    }

    /**
     * @brief block_memory block the current and futur memory pages.
     * see
     * https://wiki.linuxfoundation.org/realtime/documentation/howto/applications/memory#memory-locking
     * for further explanation.
     */
    void block_memory()
    {
        /* Lock memory */
        if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
        {
            printf("mlockall failed: %s\n", strerror(errno));
            exit(-2);
        }
    }

    bool fix_current_process_to_cpu(std::vector<int>& cpu_affinities, int pid)
    {
        if (cpu_affinities.size() > 0)
        {
            cpu_set_t mask;
            CPU_ZERO(&mask);
            for (unsigned i = 0; i < cpu_affinities.size(); ++i)
            {
                if (cpu_affinities[i] > -1)
                {
                    CPU_SET(cpu_affinities[i], &mask);
                }
            }
            pid_t process_pid = static_cast<pid_t>(pid);
            int ret = sched_setaffinity(process_pid, sizeof(mask), &mask);
            if (ret)
            {
                printf("sched_setaffinity failed. Ret= %d\n", ret);
                return false;
            }
            else
            {
                return true;
            }
        }
        printf("fix_current_process_to_cpu: Nothing to be done.\n");
        return true;
    }

    bool set_cpu_dma_latency(int max_latency_us)
    {
        int fd;

        fd = open("/dev/cpu_dma_latency", O_WRONLY);
        if (fd < 0)
        {
            perror("open /dev/cpu_dma_latency");
            return false;
        }
        if (write(fd, &max_latency_us, sizeof(max_latency_us)) !=
            sizeof(max_latency_us))
        {
            perror("write to /dev/cpu_dma_latency");
            return false;
        }

        return true;
    }

    /**
     * @brief Paramter of the real time thread
     */
    RealTimeThreadParameters parameters_;

private:
    std::unique_ptr<pthread_t> thread_;

    std::string rt_preempt_error_message_;
};

}  // namespace benchmark
}  // namespace master_board_sdk
