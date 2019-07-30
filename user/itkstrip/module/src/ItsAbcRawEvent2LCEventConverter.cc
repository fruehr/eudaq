#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"

class ItsAbcRawEvent2LCEventConverter: public eudaq::LCEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ITS_ABC");
  static const uint32_t PLANE_ID_OFFSET_ABC = 10;
};
  
namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<ItsAbcRawEvent2LCEventConverter>(ItsAbcRawEvent2LCEventConverter::m_id_factory);
}

bool ItsAbcRawEvent2LCEventConverter::Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2,
						 eudaq::ConfigSPC conf) const{
  auto raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if (!raw){
    EUDAQ_THROW("dynamic_cast error: from eudaq::Event to eudaq::RawEvent");
  }
  d2->parameters().setValue("EventType", 2);//TODO: remove
  lcio::LCCollectionVec *zsDataCollection = nullptr;
  auto p_col_names = d2->getCollectionNames();
  if(std::find(p_col_names->begin(), p_col_names->end(), "zsdata_strip") != p_col_names->end()){
    zsDataCollection = dynamic_cast<lcio::LCCollectionVec*>(d2->getCollection("zsdata_strip"));
    if(!zsDataCollection)
      EUDAQ_THROW("dynamic_cast error: from lcio::LCCollection to lcio::LCCollectionVec");
  }
  else{
    zsDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
    d2->addCollection(zsDataCollection, "zsdata_strip");
  }
  auto block_n_list = raw->GetBlockNumList();
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = raw->GetBlock(block_n);
    if(block_n > 3) continue;
    if(block_n > 1 && raw->GetRunN() >= 1347) continue; // only one hybrid running
    std::vector<bool> channels;
    eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);
    lcio::CellIDEncoder<lcio::TrackerDataImpl> zsDataEncoder("sensorID:7,sparsePixelType:5",
							     zsDataCollection);
    zsDataEncoder["sparsePixelType"] = 2;
    if(raw->GetRunN() >= 1578) block_n = block_n + 2; // only H1 running, to keep consistent plane IDs
    zsDataEncoder["sensorID"] = block_n + PLANE_ID_OFFSET_ABC;
    auto zsFrame = new lcio::TrackerDataImpl;
    zsDataEncoder.setCellID(zsFrame);
    int offset = (block_n == 2 || block_n == 3) ? 256 : 0;
    for(size_t i = 0; i < channels.size(); ++i) {
      if (channels[i]){
        zsFrame->chargeValues().push_back(i-offset);//x
        zsFrame->chargeValues().push_back(1);//y
        zsFrame->chargeValues().push_back(1);//signal
        zsFrame->chargeValues().push_back(0);//time
      }
    }
    zsDataCollection->push_back(zsFrame);
  }
  if(block_n_list.empty()){
    EUDAQ_WARN("No block in ItsAbcRawEvent EventN:TriggerN (" +
	       std::to_string(raw->GetEventN())+":"+std::to_string(raw->GetTriggerN())+")");
  }
  return true; 
}
