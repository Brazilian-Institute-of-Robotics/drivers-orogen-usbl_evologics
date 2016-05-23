#ifndef USBL_EVOLOGICS_TYPES_HPP
#define USBL_EVOLOGICS_TYPES_HPP

#include "usbl_evologics/DriverTypes.hpp"
#include "iodrivers_base/RawPacket.hpp"

namespace usbl_evologics
{
    struct DroppedMessages
    {
      base::Time time;
      SendIM droppedIm;
      std::string reason;
      unsigned long long int messageDropped;
    };

    struct DroppedRawData
    {
      base::Time time;
      iodrivers_base::RawPacket droppedData;
      std::string reason;
      unsigned long long int dataDropped;
    };

}

#endif
