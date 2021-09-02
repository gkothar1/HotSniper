#ifndef _DVFS_H_
#define _DVFS_H_

#include "performance_counters.h"
#include "policies/dvfspolicy.h"
#include "thermalModel.h"

class Dvfs
{
  public:
    Dvfs(int coreRows, int coreColumns, int minFrequency, int maxFrequency,
         int frequencyStepSize, long dvfsEpoch, int maxDVFSPatience,
         PerformanceCounters *performanceCounters, String policyName,
         float perCorePowerBudget, ThermalModel *thermalModel);

    ~Dvfs()
    {
        delete m_dvfsPolicy;
    }

    void executePolicy(std::vector<int> & oldFrequencies,
                       std::vector<bool> &activeCores);
    bool delayTransition(int coreCounter, int oldFrequency, int newFrequency);
    void TransitionDelayed(int coreCounter, int oldFrequency, int newFrequency);
    void TransitionNotDelayed(int coreCounter);
    void setFrequency(int coreCounter, int frequency);

    long
    getEpoch() const
    {
        return m_dvfsEpoch;
    }

  private:
    int                  m_num_cores;
    int                  m_minFrequency;
    int                  m_maxFrequency;
    int                  m_frequencyStepSize;
    long                 m_dvfsEpoch;
    const int            m_maxDVFSPatience = 0;
    PerformanceCounters *m_performanceCounters;
    DVFSPolicy *         m_dvfsPolicy = NULL;

    // can be used by the DVFS control loop to delay DVFS downscaling for very
    // little violations
    std::vector<int> m_downscalingPatience;

    // can be used by the DVFS control loop to delay DVFS upscaling for very
    // little violations
    std::vector<int> m_upscalingPatience;
};

#endif