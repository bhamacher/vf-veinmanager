#ifndef VEINMANAGERSETTINGS_H
#define VEINMANAGERSETTINGS_H


#include "modman_util.h"

#include <ve_eventsystem.h>
#include <ve_commandevent.h>
#include <vn_tcpsystem.h>

#include <QJsonDocument>

#include <veinmoduleentity.h>
#include <veinmodulecomponent.h>
#include <veinsharedcomp.h>
#include <veinrpcfuture.h>

namespace VeinEvent {
class StorageSystem;
}

class VeinManager : public VfCpp::VeinModuleEntity
{
    Q_OBJECT
public:
    explicit VeinManager(QObject *t_parent = nullptr);
    static constexpr int getEntityId();
    VeinEvent::StorageSystem *getStorageSystem() const;
    /**
   * @brief sets the storage system that is used to get the list of entity ids that the storage holds
   * @param t_storageSystem
   */
    void setStorage(VeinEvent::StorageSystem *t_storageSystem);

    void startServer(quint16 t_port, bool t_systemdSocket=true);

    // EventSystem interface
public:
    bool processEvent(QEvent *t_event) override;
public slots:
    /**
   * @brief connected to the signal ModuleManager::sigModulesLoaded, loads the list of entities from the m_storageSystem then sets up the data of all components
   * @param t_sessionPath
   * @param t_sessionList
   */
    void initializeEntity();
    /**
   * @brief sends add events for the entity and all components
   */
    void initOnce();

private:
     void handleAddsAndRemoves(QEvent *t_event);
private:
    static constexpr int s_entityId = 0;
    static constexpr QLatin1String s_entityName = modman_util::to_latin1("_VEIN");
    static constexpr QLatin1String s_entityNameComponentName = modman_util::to_latin1("EntityName");
    static constexpr QLatin1String s_entitiesComponentName = modman_util::to_latin1("Entities");


    VeinEvent::EventSystem* m_introspectionSystem = nullptr;;
    VeinEvent::StorageSystem *m_storageSystem = nullptr;
    VeinEvent::EventSystem* m_networkSystem = nullptr;;
    VeinNet::TcpSystem* m_tcpSystem = nullptr;;

    VfCpp::VeinSharedComp<QList<int>> m_currentEntities;
    bool m_initDone=false;



};

#endif // MODULEMANAGERSETTINGS_H
