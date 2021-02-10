#ifndef MODULEMANAGERSETTINGS_H
#define MODULEMANAGERSETTINGS_H


#include "modman_util.h"

#include <ve_eventsystem.h>
#include <ve_commandevent.h>
#include <vn_tcpsystem.h>

#include <QJsonDocument>

namespace VeinEvent {
class StorageSystem;
}

class ModuleManagerController : public VeinEvent::EventSystem
{
    Q_OBJECT
public:
    explicit ModuleManagerController(QObject *t_parent = nullptr);
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

signals:
    /**
   * @brief emitted if the session is switched via the "Session" component
   * @param t_newSession
   */
    void sigChangeSession(const QString &t_newSession);
    /**
   * @brief emitted if the modules are paused via the "ModulesPaused" component
   * @param t_paused
   */
    void sigModulesPausedChanged(bool t_paused);



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
    /**
   * @brief emits sigModulesPausedChanged
   * @param t_paused
   */
    void setModulesPaused(bool t_paused);

private:
     void handleAddsAndRemoves(QEvent *t_event);
private:
    static constexpr int s_entityId = 0;
    static constexpr QLatin1String s_entityName = modman_util::to_latin1("_VEIN");
    static constexpr QLatin1String s_entityNameComponentName = modman_util::to_latin1("EntityName");
    static constexpr QLatin1String s_entitiesComponentName = modman_util::to_latin1("Entities");


    VeinEvent::EventSystem* m_introspectionSystem = nullptr;;
    VeinEvent::StorageSystem *m_storageSystem = nullptr;
    QSet<int> m_currentEntities;
    VeinEvent::EventSystem* m_networkSystem = nullptr;;
    VeinNet::TcpSystem* m_tcpSystem = nullptr;;

    bool m_initDone=false;
    bool m_sessionReady=false;
    bool m_modulesPaused=false;



};

#endif // MODULEMANAGERSETTINGS_H
