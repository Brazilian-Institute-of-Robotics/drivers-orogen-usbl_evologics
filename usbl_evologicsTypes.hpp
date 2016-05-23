#ifndef USBL_EVOLOGICS_TYPES_HPP
#define USBL_EVOLOGICS_TYPES_HPP

#include "usbl_evologics/DriverTypes.hpp"

namespace usbl_evologics
{
    struct DroppedMessages
    {
      base::Time time;
      SendIM droppedIm;
      std::string reason;
      unsigned long long int messageDropped;
    };

}

#endif
