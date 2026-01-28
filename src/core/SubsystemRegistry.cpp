#include "SubsystemRegistry.h"
#include <fmt/core.h>
#include <algorithm>

namespace Yalaz::Core {

SubsystemRegistry::~SubsystemRegistry() {
    if (m_Initialized) {
        ShutdownAll();
    }
    Clear();
}

void SubsystemRegistry::InitAll() {
    if (m_Initialized) {
        fmt::print("[SubsystemRegistry] Warning: Already initialized\n");
        return;
    }

    fmt::print("[SubsystemRegistry] Initializing {} subsystems...\n", m_Subsystems.size());

    // Initialize in registration order (dependencies first)
    for (auto& subsystem : m_Subsystems) {
        fmt::print("[SubsystemRegistry] Initializing: {}\n", subsystem->GetName());
        subsystem->OnInit();
        subsystem->m_Initialized = true;
    }

    m_Initialized = true;
    fmt::print("[SubsystemRegistry] All subsystems initialized\n");
}

void SubsystemRegistry::ShutdownAll() {
    if (!m_Initialized) {
        return;
    }

    fmt::print("[SubsystemRegistry] Shutting down {} subsystems...\n", m_Subsystems.size());

    // Shutdown in reverse order (LIFO - dependencies last)
    for (auto it = m_Subsystems.rbegin(); it != m_Subsystems.rend(); ++it) {
        if ((*it)->IsInitialized()) {
            fmt::print("[SubsystemRegistry] Shutting down: {}\n", (*it)->GetName());
            (*it)->OnShutdown();
            (*it)->m_Initialized = false;
        }
    }

    m_Initialized = false;
    fmt::print("[SubsystemRegistry] All subsystems shut down\n");
}

void SubsystemRegistry::UpdateAll(float deltaTime) {
    for (auto& subsystem : m_Subsystems) {
        if (subsystem->IsInitialized()) {
            subsystem->OnUpdate(deltaTime);
        }
    }
}

ISubsystem* SubsystemRegistry::GetByName(const std::string& name) {
    auto it = m_NameMap.find(name);
    if (it != m_NameMap.end()) {
        return it->second;
    }
    return nullptr;
}

void SubsystemRegistry::Clear() {
    if (m_Initialized) {
        ShutdownAll();
    }

    m_TypeMap.clear();
    m_NameMap.clear();
    m_Subsystems.clear();
}

} // namespace Yalaz::Core
