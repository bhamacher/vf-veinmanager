#include "modulemanagercontroller.h"

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
constexpr QLatin1String ModuleManagerController::s_entityName;
constexpr QLatin1String ModuleManagerController::s_entityNameComponentName;
constexpr QLatin1String ModuleManagerController::s_entitiesComponentName;


ModuleManagerController::ModuleManagerController(QObject *t_parent) :
    VeinEvent::EventSystem(t_parent)
{
    QObject::connect(this,&EventSystem::sigAttached,this,&ModuleManagerController::initOnce);
    m_introspectionSystem = new VeinNet::IntrospectionSystem(this);
    m_storageSystem = new VeinStorage::VeinHash(this);
    m_networkSystem = new VeinNet::NetworkSystem(this);
    m_tcpSystem = new VeinNet::TcpSystem(this);
    connect(m_introspectionSystem,&VeinEvent::EventSystem::sigSendEvent,this,&VeinEvent::EventSystem::sigSendEvent);
    connect(m_storageSystem,&VeinEvent::EventSystem::sigSendEvent,this,&VeinEvent::EventSystem::sigSendEvent);
    connect(m_networkSystem,&VeinEvent::EventSystem::sigSendEvent,this,&VeinEvent::EventSystem::sigSendEvent);
    connect(m_tcpSystem ,&VeinEvent::EventSystem::sigSendEvent,this,&VeinEvent::EventSystem::sigSendEvent);
}

constexpr int ModuleManagerController::getEntityId()
{
    return s_entityId;
}

VeinEvent::StorageSystem *ModuleManagerController::getStorageSystem() const
{
    return m_storageSystem;
}

void ModuleManagerController::setStorage(VeinEvent::StorageSystem *t_storageSystem)
{
    m_storageSystem = t_storageSystem;
}

void ModuleManagerController::startServer(quint16 t_port, bool t_systemdSocket)
{
    m_tcpSystem->startServer(t_port,t_systemdSocket);
}

bool ModuleManagerController::processEvent(QEvent *t_event)
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


    return retVal;
}





void ModuleManagerController::initializeEntity()
{
    if(m_storageSystem!=nullptr)
    {
        m_sessionReady=true;

        VeinComponent::ComponentData *initData=nullptr;
        VeinEvent::CommandEvent *initEvent = nullptr;


        initData = new VeinComponent::ComponentData();
        initData->setEntityId(s_entityId);
        initData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        initData->setComponentName(ModuleManagerController::s_entitiesComponentName);
        initData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        initData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
        qDebug() << "ENTITIES" << m_storageSystem->getEntityList() << QVariant::fromValue<QList<int> >(m_storageSystem->getEntityList()).value<QList<int> >();
        if(m_storageSystem->getEntityList().size() > 0){
            m_currentEntities = QSet<int>(m_storageSystem->getEntityList().begin(),m_storageSystem->getEntityList().end());
        }
        initData->setNewValue(QVariant::fromValue<QList<int> >(m_currentEntities.values()));

        initEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, initData);
        emit sigSendEvent(initEvent);
        initEvent=nullptr;
    }
    else
    {
        qCritical() << "[ModuleManagerController] StorageSystem required to call initializeEntities";
    }
}

void ModuleManagerController::initOnce()
{
    if(m_initDone == false)
    {
        VeinComponent::EntityData *systemData = new VeinComponent::EntityData();
        systemData->setCommand(VeinComponent::EntityData::Command::ECMD_ADD);
        systemData->setEntityId(s_entityId);

        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, systemData));

        QVariantList ipAdressList;
        QHash<QString, QVariant> componentData;
        componentData.insert(ModuleManagerController::s_entityNameComponentName, ModuleManagerController::s_entityName);
        componentData.insert(ModuleManagerController::s_entitiesComponentName, QVariantList());

        VeinComponent::ComponentData *initialData=nullptr;
         for(const QString &compName : componentData.keys())
         {
             initialData = new VeinComponent::ComponentData();
             initialData->setEntityId(s_entityId);
             initialData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
             initialData->setComponentName(compName);
             initialData->setNewValue(componentData.value(compName));
             initialData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
             initialData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
             emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, initialData));
         }
        initializeEntity();
        m_initDone = true;
    }
}

void ModuleManagerController::setModulesPaused(bool t_paused)
{
    if(t_paused != m_modulesPaused)
    {
        m_modulesPaused = t_paused;
        emit sigModulesPausedChanged(m_modulesPaused);
    }
}

void ModuleManagerController::handleAddsAndRemoves(QEvent *t_event)
{
    VeinEvent::CommandEvent *cEvent= static_cast<VeinEvent::CommandEvent *>(t_event);
    if(cEvent->eventData()->type() == VeinComponent::EntityData::dataType()) {
        VeinComponent::EntityData* eData=static_cast<VeinComponent::EntityData*>(cEvent->eventData());
        if(eData->eventCommand() == VeinComponent::EntityData::Command::ECMD_ADD)
        {
            m_currentEntities.insert(eData->entityId());
            VeinComponent::ComponentData* entityData;
            entityData = new VeinComponent::ComponentData();
            entityData->setEntityId(s_entityId);
            entityData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            entityData->setComponentName(ModuleManagerController::s_entitiesComponentName);
            entityData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            entityData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
            entityData->setNewValue(QVariant::fromValue<QList<int> >(m_currentEntities.values()));
            VeinEvent::CommandEvent* entityEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, entityData);
            emit sigSendEvent(entityEvent );
        }
        else if(eData->eventCommand() == VeinComponent::EntityData::Command::ECMD_REMOVE)
        {
            m_currentEntities.remove(eData->entityId());
            VeinComponent::ComponentData* entityData;
            entityData = new VeinComponent::ComponentData();
            entityData->setEntityId(s_entityId);
            entityData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            entityData->setComponentName(ModuleManagerController::s_entitiesComponentName);
            entityData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            entityData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
            entityData->setNewValue(QVariant::fromValue<QList<int> >(m_currentEntities.values()));
            VeinEvent::CommandEvent* entityEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, entityData);
            emit sigSendEvent(entityEvent );
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



