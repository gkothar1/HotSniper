#include "dvfs.h"
#include "magic_server.h"
#include "simulator.h"
#include "config.hpp"
#include "policies/dvfsFixedPower.h"
#include "policies/dvfsMaxFreq.h"
#include "policies/dvfsTSP.h"
#include "policies/dvfsTestStaticPower.h"

Dvfs::Dvfs(int coreRows, int coreColumns, int minFrequency, int maxFrequency,
           int frequencyStepSize, long dvfsEpoch, int maxDVFSPatience,
           PerformanceCounters *performanceCounters, String policyName,
           float perCorePowerBudget, ThermalModel *thermalModel)
    : m_num_cores(coreRows * coreColumns), m_minFrequency(minFrequency),
      m_maxFrequency(maxFrequency), m_frequencyStepSize(frequencyStepSize),
      m_dvfsEpoch(dvfsEpoch), m_maxDVFSPatience(maxDVFSPatience),
      m_performanceCounters(performanceCounters)
{
    std::cout << "[Scheduler] [Dvfs] [Info]: Initializing DVFS policy"
              << std::endl;

    for (core_id_t core_id = 0; core_id < m_num_cores; core_id++)
    {
        m_downscalingPatience.push_back(m_maxDVFSPatience);
        m_upscalingPatience.push_back(m_maxDVFSPatience);
    }

    if (policyName == "maxFreq")
    {
        m_dvfsPolicy = new DVFSMaxFreq(m_performanceCounters, coreRows,
                                       coreColumns, m_maxFrequency);
    }
    else if (policyName == "testStaticPower")
    {
        m_dvfsPolicy = new DVFSTestStaticPower(m_performanceCounters, coreRows,
                                               coreColumns, minFrequency,
                                               m_maxFrequency);
    }
    else if (policyName == "fixedPower")
    {
        float perCorePowerBudget = Sim()->getCfg()->getFloat(
            "scheduler/dvfs/fixed_power/per_core_power_budget");
        m_dvfsPolicy = new DVFSFixedPower(
            m_performanceCounters, coreRows, coreColumns, minFrequency,
            m_maxFrequency, m_frequencyStepSize, perCorePowerBudget);
    }
    else if (policyName == "tsp")
    {
        m_dvfsPolicy = new DVFSTSP(thermalModel, m_performanceCounters,
                                   coreRows, coreColumns, minFrequency,
                                   m_maxFrequency, m_frequencyStepSize);
    }
    else
    {
        std::cout << "\n[Scheduler] [Dvfs] [Error]: Unknown DVFS Algorithm"
                  << std::endl;
        exit(1);
    }
}

/**
 * Return whether the DVFS control loop should be patient and delay the DVFS
 * scaling.
 */
bool
Dvfs::delayTransition(int coreCounter, int oldFrequency, int newFrequency)
{
    return ((newFrequency == (oldFrequency - m_frequencyStepSize))
            && (m_downscalingPatience.at(coreCounter) > 0))
           || ((newFrequency == (oldFrequency + m_frequencyStepSize))
               && (m_upscalingPatience.at(coreCounter) > 0));
}

/**
 * Notify that a DVFS transition was delayed
 */
void
Dvfs::TransitionDelayed(int coreCounter, int oldFrequency, int newFrequency)
{
    if (newFrequency == (oldFrequency - m_frequencyStepSize))
    {
        std::cout << "DVFS transition delayed (current patience: "
                  << m_downscalingPatience.at(coreCounter) << ")" << std::endl;
        m_downscalingPatience.at(coreCounter) -= 1;
    }
    else if (newFrequency == (oldFrequency + m_frequencyStepSize))
    {
        std::cout << "DVFS transition delayed (current patience: "
                  << m_upscalingPatience.at(coreCounter) << ")" << std::endl;
        m_upscalingPatience.at(coreCounter) -= 1;
    }
}

/**
 * Notify that a DVFS transition was not delayed
 */
void
Dvfs::TransitionNotDelayed(int coreCounter)
{
    m_downscalingPatience.at(coreCounter) = m_maxDVFSPatience;
    m_upscalingPatience.at(coreCounter)   = m_maxDVFSPatience;
}

/**
 * Set the frequency for a core.
 */
void
Dvfs::setFrequency(int coreCounter, int frequency)
{
    int oldFrequency = Sim()->getMagicServer()->getFrequency(coreCounter);

    if (frequency > (oldFrequency + 1000))
    {
        frequency = oldFrequency + 1000;
    }
    if (frequency < m_minFrequency)
    {
        frequency = m_minFrequency;
    }
    if (frequency > m_maxFrequency)
    {
        frequency = m_maxFrequency;
    }

    if (delayTransition(coreCounter, oldFrequency, frequency))
    {
        TransitionDelayed(coreCounter, oldFrequency, frequency);
    }
    else
    {
        TransitionNotDelayed(coreCounter);
        if (frequency != oldFrequency)
        {
            Sim()->getMagicServer()->setFrequency(coreCounter, frequency);
        }
    }
}

/** executeDVFSPolicy
 * Set DVFS levels according to the used policy.
 */
void
Dvfs::executePolicy(std::vector<int> & oldFrequencies,
                    std::vector<bool> &activeCores)
{
    std::vector<int> frequencies
        = m_dvfsPolicy->getFrequencies(oldFrequencies, activeCores);
    for (int coreCounter = 0; coreCounter < m_num_cores; coreCounter++)
    {
        setFrequency(coreCounter, frequencies.at(coreCounter));
    }
    m_performanceCounters->notifyFreqsOfCores(frequencies);
}