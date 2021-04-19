#include <memory>

#include "Config.hpp"
#include "OutputHandler.hpp"
#include "Malt.hpp"
#include "MaltSender.hpp"
#include "MaltReceiver.hpp"
#include "ReceiverPolicyRaw.hpp"
#include "ReceiverPolicyReg.hpp"

namespace malt {

std::unique_ptr<IMaltRunner> makeRunner(
        Config const& cfg, OutputHandler& oh, bool& stopped) {
    if (cfg.sender())
        return std::make_unique<MaltSender>(cfg, oh, stopped);
    
    if (cfg.wildcard())
        return std::make_unique<MaltReceiver<ReceiverPolicyRaw>>(cfg, oh, stopped);
    
    return std::make_unique<MaltReceiver<ReceiverPolicyReg>>(cfg, oh, stopped);
}


} // namespace malt