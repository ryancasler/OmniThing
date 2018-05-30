#include "OmniThing.h"
#include "OmniUtil.h"

#include "Device.h"
#include "DeviceConfig.h"
#include "NetworkReceiver.h"
#include "NetworkSender.h"
#include "Logger.h"
#include "frozen.h"
#include <string.h>

namespace omni
{
//private
    OmniThing::OmniThing():
        m_nDeviceCount(0),
        m_nTriggerCount(0),
        m_nDeviceConfigCount(0),
        m_pNetworkReceiver(nullptr),
        m_pNetworkSender(nullptr),
        m_pDefaultLogger(new Logger()),
        m_pLogger(m_pDefaultLogger)
    {

    }

    void OmniThing::runScheduler()
    {
        //run all run-enabled devices
        for(unsigned short i = 0; i < m_nDeviceCount; ++i)
        {
            if(m_Devices[i]->isRunning())
            {
                m_Devices[i]->run();
            }
        }

        //check for triggers
        //TODO: account for overflow in the for loop
        //TODO: account for non-repeating triggers
        unsigned long long time = getMillis();
        for(unsigned short i = 0; i < m_nTriggerCount; ++i)
        {
            Trigger& t = m_Triggers[i];
            
            if(time >= t.triggerTime)
            {
                t.triggerTime += t.interval;
                t.dev->recvJson(t.cmd, t.json);
            }
        }
    }

    void OmniThing::initDevices()
    {
        LOG << F("Initializing devices...\n");

        //run init on all devices
        for(unsigned short i = 0; i < m_nDeviceCount; ++i)
        {
            m_Devices[i]->init();
        }

    }

    void OmniThing::initScheduler()
    {
        LOG << F("Initializing scheduler...\n");

        unsigned long long time = getMillis();
        for(unsigned short i = 0; i < m_nTriggerCount; ++i)
        {
            Trigger& t = m_Triggers[i];
            
            t.triggerTime = time + t.interval;
        }

    }

    void OmniThing::parseJson(const char* json)
    {
        unsigned int uid;
        char cmd[24]; 
        
        int res = json_scanf(json, strlen(json), F("{uid:%u, cmd:%s}"), &uid, cmd);
        if(res != 2)
        {
            LOG << F("problem scanning err=") << res << F(" : ") << json << Logger::endl;
        }

        Device* d = findDevice(uid);
        if(!d)
        {
            LOG << F("No device found with uid=") << uid << Logger::endl;
        }
        else
        {
            d->recvJson(cmd, json);
        }
    }

    Device* OmniThing::findDevice(unsigned int uid)
    {
        for(unsigned int i = 0; i < m_nDeviceCount; ++ i)
        {
            Device* d = m_Devices[i];
            if(d->getUid() == uid)
                return d;
        }

        return nullptr;
    }

//public
    OmniThing& OmniThing::getInstance()
    {
        static OmniThing instance;

        return instance;
    }

    void OmniThing::init()
    {
        initDevices(); 
        initScheduler();

        if(m_pNetworkSender)
            m_pNetworkSender->init();
        else
            LOG << F("WARNING: no NetworkSender configured\n");

        if(m_pNetworkReceiver)
            m_pNetworkReceiver->init();
        else
            LOG << F("WARNING: no NetworkReceiver configured\n");

        // Provide some debug output
        LOG << F("Device Configurations:\n");
        for(unsigned int i = 0; i < m_nDeviceConfigCount; ++i)
        {
            LOG << F("\t") << m_DeviceConfigs[i]->getType() << F("\n");
        }
    }

    void OmniThing::run()
    {
        runScheduler();

        if(m_pNetworkSender)
        {
            m_pNetworkSender->run();
        }

        if(m_pNetworkReceiver)
        {
            m_pNetworkReceiver->run();

            const char* json_rcvd = m_pNetworkReceiver->getJsonString();
            if(json_rcvd)
            {
                LOG << F("Received json: ") << json_rcvd << Logger::endl;
                parseJson(json_rcvd);
            }
        }
    }

    void OmniThing::setNetworkReceiver(NetworkReceiver* nr)
    {
        m_pNetworkReceiver = nr;
    }

    void OmniThing::setNetworkSender(NetworkSender* ns)
    {
        m_pNetworkSender = ns;
    }

    void OmniThing::setLogger(Logger* l)
    {
        m_pLogger = l;
    }

    Logger& OmniThing::getLogger()
    {
        return *m_pLogger;
    }

    void OmniThing::sendJson(const char* json)
    {
        if(m_pNetworkSender)
        {
            m_pNetworkSender->sendJson(json);
        }
    } 

    void OmniThing::addDevice(Device* dev)
    {
        m_Devices[m_nDeviceCount++] = dev;
    }

    void OmniThing::addTrigger(Trigger& t)
    {
        m_Triggers[m_nTriggerCount++] = t;
    }

    void OmniThing::addTrigger(Device* d, unsigned long interval, const char* cmd, const char* json, bool repeat)
    {
        m_Triggers[m_nTriggerCount++] = Trigger(d, interval, cmd, json, repeat);
    }

    void OmniThing::addDeviceConfig(DeviceConfig* dc)
    {
        m_DeviceConfigs[m_nDeviceConfigCount++] = dc;
    }
}
