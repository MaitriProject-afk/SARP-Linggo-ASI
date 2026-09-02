#pragma once

namespace SARPLinggo {

class D3D9Hook {
public:
    static bool init();
    static void shutdown();
    static bool is_ready() { return m_ready; }

private:
    static bool m_ready;
};

} // namespace SARPLinggo
