#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "fixed_types.h"
#include "thread_manager.h"

#include "dvfs.h"
#include "performance_counters.h"
#include "thermalModel.h"

class Scheduler
{
   public:
      static Scheduler* create(ThreadManager *thread_manager);

      Scheduler(ThreadManager *thread_manager);
      virtual ~Scheduler() { if (m_dvfs) delete m_dvfs; }

      virtual core_id_t threadCreate(thread_id_t thread_id) = 0;
      virtual void threadYield(thread_id_t thread_id) {}
      virtual bool threadSetAffinity(thread_id_t calling_thread_id, thread_id_t thread_id, size_t cpusetsize, const cpu_set_t *mask) { return false; }
      virtual bool threadGetAffinity(thread_id_t thread_id, size_t cpusetsize, cpu_set_t *mask) { return false; }

      // override as required for the chosen scheduler algorithm
      virtual void executeDVFSPolicy(){}

   protected:
      ThreadManager *m_thread_manager;

      // Utility functions
      core_id_t findFirstFreeCore();
      void printMapping();

      // Scheduler based DVFS
      Dvfs *               m_dvfs;
      int                  m_coreRows;
      int                  m_coreColumns;
      ThermalModel *       m_thermalModel;
      PerformanceCounters *m_performanceCounters;
      bool                 m_dvfs_enabled;
};

#endif // __SCHEDULER_H
