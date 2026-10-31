//
// Created by SuperTuxi on 13.06.25.
//

#ifndef PIPEWIREAPI_H
#define PIPEWIREAPI_H

#include <thread>
#include <pipewire/pipewire.h>
#include <pipewire/impl-module.h>
#include <QStandardItemModel>

class MainWindow;
class PipewireAPI;

struct pw_class {
    const char *type;
    uint32_t version;
    const void *events;
    pw_destroy_t destroy;
    // info_func_t info;
    const char *name_key;
};

struct global {
    struct remote_data* remote_data;
    uint32_t id;
    uint32_t permissions;
    uint32_t version;
    char* type;
    const struct pw_class* pw_class;
    struct pw_proxy* proxy;
    // bool info_pending;
    struct pw_properties* properties;
    QStandardItem* modelItem;
};

struct proxy_data {
    struct remote_data* remote_data;
    struct global* global;
    struct pw_proxy* proxy;
    void* info;
    const struct pw_class* pw_class;
    struct spa_hook proxy_listener;
    struct spa_hook object_listener;
    uint32_t id;
    bool handled;
};

struct data {
    struct pw_thread_loop* thread_loop;
    struct pw_loop* loop;
    struct pw_context* context;

    struct remote_data* remote_data;
};

struct remote_data {
    PipewireAPI* instance;
    struct data* data;

    struct pw_core* core;
    struct spa_hook core_listener;
    struct spa_hook proxy_core_listener;
    struct pw_registry* registry;
    struct spa_hook registry_listener;

    struct pw_map globals;
    std::map<uint32_t, std::pair<std::list<uint32_t>, std::list<uint32_t>>> *ports;
    std::map<std::pair<uint32_t, uint32_t>, std::list<uint32_t>> *links;
    bool cleanup;
    bool verbose;
};

class PipewireAPI {
public:
    PipewireAPI();
    ~PipewireAPI();

    void run(int argc, char *argv[]);
    void quit() const;

    void loadSink(const std::string& name) const;
    void loadSource(const std::string& name) const;
    void unloadModule(const std::string& id) const;
    const char* getId(const struct global* global) const;
    void linkNodes(uint32_t outputId, uint32_t inputId) const;
    void unlinkNodes(uint32_t outputId, uint32_t inputId) const;
    void setSinkVolume(const std::string &name, int value) const;
    std::string info_node(const struct proxy_data *pd) const;
    static std::string info_global(const struct proxy_data *pd);
    static std::string info_properties(const struct spa_dict *props);

    [[nodiscard]] QStandardItemModel* inputModel() const {
        return inputs;
    }
    [[nodiscard]] MainWindow* main_window() const {
        return mainWindow;
    }
    void set_main_window(MainWindow* main_window) {
        mainWindow = main_window;
    }
private:
    std::thread thread;
    struct data* data;
    struct remote_data* remote_data;

    MainWindow* mainWindow;
    QStandardItemModel* inputs;

    void createLink(const struct pw_properties* props) const;
};

#endif //PIPEWIREAPI_H
