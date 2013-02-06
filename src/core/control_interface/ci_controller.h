/**
 * @file <argos3/core/control_interface/ci_controller.h>
 *
 * @author Carlo Pinciroli - <ilpincy@gmail.com>
 */

#ifndef CCI_CONTROLLER_H
#define CCI_CONTROLLER_H

namespace argos {
   class CCI_Controller;
}

#include <argos3/core/utility/configuration/base_configurable_resource.h>
#include <argos3/core/utility/datatypes/datatypes.h>
#include <argos3/core/control_interface/ci_sensor.h>
#include <argos3/core/control_interface/ci_actuator.h>
#include <argos3/core/utility/plugins/factory.h>

#include <map>
#include <string>
#include <cxxabi.h>
#include <typeinfo>

namespace argos {

   class CCI_Controller : public CBaseConfigurableResource {

   public:

      virtual ~CCI_Controller();

      virtual void Init(TConfigurationNode& t_node) {}

      virtual void ControlStep() {}

      virtual void Reset() {}

      virtual void Destroy() {}

      inline const std::string& GetId() const {
         return m_strId;
      }

      inline void SetId(const std::string& str_id) {
         m_strId = str_id;
      }

      virtual bool IsControllerFinished() const {
         return false;
      }

      template<typename ACTUATOR_IMPL>
      ACTUATOR_IMPL* GetActuator(const std::string& str_actuator_type) {
         CCI_Actuator::TMap::const_iterator it = m_mapActuators.find(str_actuator_type);
         if (it != m_mapActuators.end()) {
            ACTUATOR_IMPL* pcActuator = dynamic_cast<ACTUATOR_IMPL*>(it->second);
            if(pcActuator != NULL) {
               return pcActuator;
            }
            else {
               char* pchDemangledType = abi::__cxa_demangle(typeid(ACTUATOR_IMPL).name(), NULL, NULL, NULL);
               THROW_ARGOSEXCEPTION("Actuator type " << str_actuator_type << " cannot be cast to type " << pchDemangledType);
            }
         }
         else {
            THROW_ARGOSEXCEPTION("Unknown actuator type " << str_actuator_type << " requested in controller. Did you add it to the XML file?");
         }
      }

      template<typename SENSOR_IMPL>
      SENSOR_IMPL* GetSensor(const std::string& str_sensor_type) {
         CCI_Sensor::TMap::const_iterator it = m_mapSensors.find(str_sensor_type);
         if (it != m_mapSensors.end()) {
            SENSOR_IMPL* pcSensor = dynamic_cast<SENSOR_IMPL*>(it->second);
            if(pcSensor != NULL) {
               return pcSensor;
            }
            else {
               char* pchDemangledType = abi::__cxa_demangle(typeid(SENSOR_IMPL).name(), NULL, NULL, NULL);
               THROW_ARGOSEXCEPTION("Sensor type " << str_sensor_type << " cannot be cast to type " << pchDemangledType);
            }
         }
         else {
            THROW_ARGOSEXCEPTION("Unknown sensor type " << str_sensor_type << " requested in controller. Did you add it to the XML file?");
         }
      }

      bool HasActuator(const std::string& str_id) const;

      bool HasSensor(const std::string& str_id) const;

      inline CCI_Actuator::TMap& GetAllActuators() {
    	  return m_mapActuators;
      }

      inline CCI_Sensor::TMap& GetAllSensors() {
    	  return m_mapSensors;
      }

      inline void AddActuator(const std::string& str_actuator_type,
                              CCI_Actuator* pc_actuator) {
         m_mapActuators[str_actuator_type] = pc_actuator;
      }

      inline void AddSensor(const std::string& str_sensor_type,
                            CCI_Sensor* pc_sensor) {
         m_mapSensors[str_sensor_type] = pc_sensor;
      }

   protected:

      CCI_Actuator::TMap m_mapActuators;
      CCI_Sensor::TMap m_mapSensors;

      std::string m_strId;

   };

}

#define REGISTER_CONTROLLER(CLASSNAME, LABEL) \
   REGISTER_SYMBOL(CCI_Controller,            \
                   CLASSNAME,                 \
                   LABEL,                     \
                   "undefined",               \
                   "undefined",               \
                   "undefined",               \
                   "undefined",               \
                   "undefined")

#endif
