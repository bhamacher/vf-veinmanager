#include "veinmanager.h"

#include <ve_eventdata.h>
#include <ve_commandevent.h>
#include <ve_storagesystem.h>
#include <vcmp_componentdata.h>
#include <vcmp_entitydata.h>
#include <vcmp_introspectiondata.h>

#include <vn_networksystem.h>
#include <vn_tcpsystem.h>
#include <vn_introspectionsystem.h>
#include <vs_veinhash.h>


#include <QJsonArray>
#include <QDateTime>

//constexpr definition, see: https://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char
constexpr QLatin1String VeinManager::s_entityName;
constexpr QLatin1String VeinManager::s_entityNameComponentName;
constexpr QLatin1String VeinManager::s_entitiesComponentName;


VeinManager::VeinManager(QObject *t_parent) :
    VfCpp::VeinModuleEntity(0,t_parent)
{
    QObject::connect(this,&EventSystem::sigAttached,this,&VeinManager::initOnce);
    m_introspectionSystem = new VeinNet::IntrospectionSystem(this);
    m_storageSystem = new VeinStorage::VeinHash(this);
    m_networkSystem = new VeinNet::NetworkSystem(this);
    m_tcpSystem = new VeinNet::TcpSystem(this);
    connect(m_introspectionSystem,&VeinEvent::EventSystem::sigSendEvent,this,&VeinEvent::EventSystem::sigSendEvent);
    connect(m_storageSystem,&VeinEvent::EventSystem::sigSendEvent,this,&VeinEvent::EventSystem::sigSendEvent);
    connect(m_networkSystem,&VeinEvent::EventSystem::sigSendEvent,this,&VeinEvent::EventSystem::sigSendEvent);
    connect(m_tcpSystem ,&VeinEvent::EventSystem::sigSendEvent,this,&VeinEvent::EventSystem::sigSendEvent);
}

constexpr int VeinManager::getEntityId()
{
    return s_entityId;
}

VeinEvent::StorageSystem *VeinManager::getStorageSystem() const
{
    return m_storageSystem;
}

void VeinManager::setStorage(VeinEvent::StorageSystem *t_storageSystem)
{
    m_storageSystem = t_storageSystem;
}

void VeinManager::startServer(quint16 t_port, bool t_systemdSocket)
{
    m_tcpSystem->startServer(t_port,t_systemdSocket);
}

bool VeinManager::processEvent(QEvent *t_event)
{
    bool retVal = false;


    if(t_event->type() == VeinEvent::CommandEvent::eventType())
    {
        bool validated=false;
        VeinEvent::CommandEvent *cEvent = nullptr;
        cEvent = static_cast<VeinEvent::CommandEvent *>(t_event);
        Q_ASSERT(cEvent != nullptr);
        if(cEvent->eventSubtype() != VeinEvent::CommandEvent::EventSubtype::NOTIFICATION) //we do not need to process notifications
        {
            if(cEvent->eventData()->type() == VeinComponent::IntrospectionData::dataType()) //introspection requests are always valid
            {
                validated = true;
            }
            else if(cEvent->eventData()->type() == VeinComponent::EntityData::dataType()) //validate subscription requests
            {
                VeinComponent::EntityData *eData=nullptr;
                eData = static_cast<VeinComponent::EntityData *>(cEvent->eventData());
                Q_ASSERT(eData != nullptr);
                if(eData->eventCommand() == VeinComponent::EntityData::Command::ECMD_SUBSCRIBE
                        || eData->eventCommand() == VeinComponent::EntityData::Command::ECMD_UNSUBSCRIBE) /// @todo maybe add roles/views later
                {
                    validated = true;
                }
            }
            else if(cEvent->eventData()->type() == VeinComponent::ComponentData::dataType())
            {
                VeinComponent::ComponentData *cData=nullptr;
                cData = static_cast<VeinComponent::ComponentData *>(cEvent->eventData());
                Q_ASSERT(cData != nullptr);

                //validate fetch requests
                if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_FETCH) /// @todo maybe add roles/views later
                {
                    validated = true;
                }

            }

        }
        if(validated == true)
        {
            ///@todo @bug remove inconsistent behavior by sending a new event instead of rewriting the current event
            retVal = true;
            cEvent->setEventSubtype(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION);
            cEvent->eventData()->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL); //the validated answer is authored from the system that runs the validator (aka. this system)
            cEvent->eventData()->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL); //inform all users (may or may not result in network messages)
        }
        handleAddsAndRemoves(cEvent);
    }


    m_introspectionSystem->processEvent(t_event);
    m_storageSystem->processEvent(t_event);
    m_networkSystem->processEvent(t_event);
    m_tcpSystem->processEvent(t_event);

    VfCpp::VeinModuleEntity::processEvent(t_event);


    return retVal;
}





void VeinManager::initializeEntity()
{
    if(m_storageSystem!=nullptr)
    {
        qDebug() << "ENTITIES" << m_storageSystem->getEntityList() << QVariant::fromValue<QList<int> >(m_storageSystem->getEntityList()).value<QList<int> >();
        if(m_storageSystem->getEntityList().size() > 0){
            m_currentEntities.setValue(QSet<int>(m_storageSystem->getEntityList().begin(),m_storageSystem->getEntityList().end()).values());
        }
    }
    else
    {
        qCritical() << "[ModuleManagerController] StorageSystem required to call initializeEntities";
    }
}

void VeinManager::initOnce()
{
    if(m_initDone == false)
    {
        createComponent("EntityName","_VEIN",VfCpp::cVeinModuleComponent::Direction::constant);
        m_currentEntities=createComponent("Entities",QVariantList(),VfCpp::cVeinModuleComponent::Direction::out);
        initializeEntity();
        m_initDone = true;
    }
}

void VeinManager::handleAddsAndRemoves(QEvent *t_event)
{
    VeinEvent::CommandEvent *cEvent= static_cast<VeinEvent::CommandEvent *>(t_event);
    if(cEvent->eventData()->type() == VeinComponent::EntityData::dataType()) {
        VeinComponent::EntityData* eData=static_cast<VeinComponent::EntityData*>(cEvent->eventData());
        if(eData->eventCommand() == VeinComponent::EntityData::Command::ECMD_ADD)
        {
            QSet<int> tmpSet=QSet<int>(m_currentEntities.value().begin(),m_currentEntities.value().end());
            tmpSet.insert(eData->entityId());
            m_currentEntities=tmpSet.values();
        }
        else if(eData->eventCommand() == VeinComponent::EntityData::Command::ECMD_REMOVE)
        {
            QSet<int> tmpSet=QSet<int>(m_currentEntities.value().begin(),m_currentEntities.value().end());
            tmpSet.remove(eData->entityId());
            m_currentEntities=tmpSet.values();
        }
    }
    else if(cEvent->eventData()->type() == VeinComponent::ComponentData::dataType()){
        VeinComponent::ComponentData* cData=static_cast<VeinComponent::ComponentData*>(cEvent->eventData());
        if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_ADD)
        {
            // nothing to do
        }
        else if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_REMOVE)
        {
            // nothing to do
        }

    }

}



