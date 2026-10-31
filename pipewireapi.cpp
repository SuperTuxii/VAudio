//
// Created by SuperTuxi on 13.06.25.
//

#include "pipewireapi.h"

#include "mainwindow.h"

void node_event_info(void *data, const struct pw_node_info *info) {
    auto* proxy_data = static_cast<struct::proxy_data*>(data);
    struct remote_data* remote_data = proxy_data->remote_data;
    proxy_data->info = pw_node_info_update(static_cast<struct::pw_node_info*>(proxy_data->info), info);
    if (!proxy_data->global) {
        proxy_data->global = static_cast<struct::global*>(pw_map_lookup(&remote_data->globals, info->id));
    }
    if (proxy_data->handled)
        return;
    spa_dict *props = static_cast<struct::pw_node_info*>(proxy_data->info)->props;
    bool isModule = spa_streq(spa_dict_lookup(props, PW_KEY_APP_NAME), "VAudio");
    if (isModule && spa_dict_lookup(&proxy_data->global->properties->dict, "vaudio.cleanup")) {
        if (pw_properties_get_bool(proxy_data->global->properties, "vaudio.cleanup", false)) {
            const char* id = remote_data->instance->getId(proxy_data->global);
            pw_properties_set(proxy_data->global->properties, "vaudio.cleanup", "false");
            if (remote_data->verbose)
                fprintf(stdout, "cleaning up \"%s\" (%s) with id %s\n",
                    spa_dict_lookup(props, PW_KEY_NODE_NAME),
                    spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION),
                    id);
            remote_data->instance->unloadModule(id);
        }
        return;
    }
    if (remote_data->ports->contains(proxy_data->global->id)) {
        if (isModule) {
            if (spa_streq(spa_dict_lookup(props, PW_KEY_MEDIA_CLASS), "Audio/Sink")) {
                if (remote_data->verbose)
                    fprintf(stdout, "sink for \"%s\" (%s) registered\n",
                        spa_dict_lookup(props, PW_KEY_NODE_NAME),
                        spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION));
                remote_data->instance->main_window()->setSinkPanelSink(proxy_data->global);
            } else {
                if (remote_data->verbose)
                    fprintf(stdout, "source for \"%s\" (%s) registered\n",
                        spa_dict_lookup(props, PW_KEY_NODE_NAME),
                        spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION));
                remote_data->instance->main_window()->setSinkPanelSource(proxy_data->global);
            }
        } else if (proxy_data->global->modelItem) {
            remote_data->instance->main_window()->inputOptionInserted(proxy_data);
            if (remote_data->verbose)
                fprintf(stdout, "new selectable node \"%s\" handled\n",
                    proxy_data->global->modelItem->text().toStdString().c_str());
        }
        proxy_data->handled = true;
    }
}
void port_event_info(void *data, const struct pw_port_info *info) {
    auto* proxy_data = static_cast<struct::proxy_data*>(data);
    proxy_data->info = pw_port_info_update(static_cast<struct::pw_port_info*>(proxy_data->info), info);
    if (!proxy_data->global)
        proxy_data->global = static_cast<struct::global*>(pw_map_lookup(&proxy_data->remote_data->globals, info->id));
}
void link_event_info(void *data, const struct pw_link_info *info) {
    auto* proxy_data = static_cast<struct::proxy_data*>(data);
    proxy_data->info = pw_link_info_update(static_cast<struct::pw_link_info*>(proxy_data->info), info);
    if (!proxy_data->global)
        proxy_data->global = static_cast<struct::global*>(pw_map_lookup(&proxy_data->remote_data->globals, info->id));
}

constexpr struct pw_node_events node_events = {
    .version = PW_VERSION_NODE_EVENTS,
    .info = node_event_info
};
constexpr struct pw_port_events port_events = {
    .version = PW_VERSION_PORT_EVENTS,
    .info = port_event_info,
};
constexpr struct pw_link_events link_events = {
    .version = PW_VERSION_LINK_EVENTS,
    .info = link_event_info
};

static const struct pw_class node_class = {
    .type = PW_TYPE_INTERFACE_Node,
    .version = PW_VERSION_NODE,
    .events = &node_events,
    .destroy = reinterpret_cast<pw_destroy_t>(pw_node_info_free),
    // .info = info_node,
    .name_key = PW_KEY_NODE_NAME
};
static const struct pw_class port_class = {
    .type = PW_TYPE_INTERFACE_Port,
    .version = PW_VERSION_PORT,
    .events = &port_events,
    .destroy = reinterpret_cast<pw_destroy_t>(pw_port_info_free),
    // .info = info_port,
};
static const struct pw_class link_class = {
    .type = PW_TYPE_INTERFACE_Link,
    .version = PW_VERSION_LINK,
    .events = &link_events,
    .destroy = reinterpret_cast<pw_destroy_t>(pw_link_info_free),
    // .info = info_link,
};

static const struct pw_class* classes[] = {
    &node_class,
    &port_class,
    &link_class
};

static const struct pw_class *find_class(const char *type, const uint32_t version) {
    for (const auto pw_class : classes) {
        if (spa_streq(pw_class->type, type) &&
            pw_class->version <= version)
            return pw_class;
    }
    return nullptr;
}

void removed_proxy(void* data) {
    auto* proxy_data = static_cast<struct::proxy_data*>(data);
    pw_proxy_destroy(proxy_data->proxy);
}

void destroy_proxy(void* data) {
    auto* proxy_data = static_cast<struct::proxy_data*>(data);

    spa_hook_remove(&proxy_data->proxy_listener);
    spa_hook_remove(&proxy_data->object_listener);

    if (proxy_data->global)
        proxy_data->global->proxy = nullptr;

    if (!proxy_data->info)
        return;

    if (proxy_data->pw_class->destroy)
        proxy_data->pw_class->destroy(proxy_data->info);
    proxy_data->info = nullptr;
}

constexpr pw_proxy_events proxy_events = {
    .version = PW_VERSION_PROXY_EVENTS,
    .destroy = destroy_proxy,
    .removed = removed_proxy
};

bool strContains(const char* str, const char* ptr) {
    if (!str)
        return false;
    return std::strstr(str, ptr);
}

bool strPrefix(const char* str, const char* pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}


void registry_event_global(void *data, const uint32_t id,
                           const uint32_t permissions, const char* type,
                           const uint32_t version, const struct spa_dict* props) {
    auto* remote_data = static_cast<struct::remote_data*>(data);
    auto* global = new struct::global;
    global->remote_data = remote_data;
    global->id = id;
    global->permissions = permissions;
    global->type = strdup(type);
    global->version = version;
    global->properties = props ? pw_properties_new_dict(props) : nullptr;
    global->modelItem = nullptr;

    size_t size = pw_map_get_size(&remote_data->globals);
    while (id > size)
        pw_map_insert_at(&remote_data->globals, size++, nullptr);
    pw_map_insert_at(&remote_data->globals, id, global);

    const struct pw_class* pw_class = find_class(type, version);
    if (!pw_class) {
        // fprintf(stdout, "unsupported type %s\n", type);
        return;
    }
    global->pw_class = pw_class;

    auto* proxy = static_cast<struct::pw_proxy*>(pw_registry_bind(remote_data->registry,
                                                    id, type, version,
                                                    sizeof(struct proxy_data)));

    auto* proxy_data = static_cast<struct::proxy_data*>(pw_proxy_get_user_data(proxy));
    proxy_data->remote_data = remote_data;
    proxy_data->global = global;
    proxy_data->proxy = proxy;
    proxy_data->pw_class = pw_class;
    pw_proxy_add_object_listener(proxy, &proxy_data->object_listener, pw_class->events, proxy_data);
    pw_proxy_add_listener(proxy, &proxy_data->proxy_listener, &proxy_events, proxy_data);

    global->proxy = proxy;

    if (spa_streq(type, PW_TYPE_INTERFACE_Node)) {
        proxy_data->handled = false;
        if (remote_data->cleanup && spa_streq(spa_dict_lookup(props, PW_KEY_APP_NAME), "VAudio")) {
            if (remote_data->verbose)
                fprintf(stdout, "marking cleanup for %s (%s)\n",
                    spa_dict_lookup(props, PW_KEY_NODE_NAME),
                    spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION));
            pw_properties_set(global->properties, "vaudio.cleanup", "true");
            return;
        }
        const char* media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        if (!strContains(media_class, "Audio") &&
            !strContains(spa_dict_lookup(props, PW_KEY_MEDIA_TYPE), "Audio"))
            return;
        QString node_name = "Unknown Name";
        if (spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION)) {
            node_name = QString(spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION));
        } else if (spa_dict_lookup(props, PW_KEY_NODE_NAME)) {
            node_name = QString(spa_dict_lookup(props, PW_KEY_NODE_NAME));
        }
        global->modelItem = new QStandardItem(node_name);
        QVariant userData;
        userData.setValue(global);
        global->modelItem->setData(userData, Qt::UserRole);
        if (media_class) {
            QIcon icon = QIcon(":/icons/preferences-desktop-display");
            if (strPrefix(media_class, "Audio/Sink"))
                icon = QIcon(":/icons/audio-volume-high");
            else if (strPrefix(media_class, "Audio/Source"))
                icon = QIcon(":/icons/audio-input-microphone");
            else if (strPrefix(media_class, "Audio/Duplex"))
                icon = QIcon(":/icons/audio-duplex");
            else if (strPrefix(media_class, "Stream/Output/Audio"))
                icon = QIcon(":/icons/preferences-desktop-display-speaker");
            else if (strPrefix(media_class, "Stream/Input/Audio"))
                icon = QIcon(":/icons/preferences-desktop-display-microphone");
            global->modelItem->setData(icon, Qt::DecorationRole);
        }
        if (spa_dict_lookup(props, PW_KEY_NODE_NICK))
            global->modelItem->setToolTip(QString(spa_dict_lookup(props, PW_KEY_NODE_NICK)));
        QStandardItemModel* model = remote_data->instance->inputModel();
        model->insertRow(model->rowCount(), proxy_data->global->modelItem);
        if (remote_data->verbose)
            fprintf(stdout, "new selectable node \"%s\" registered\n", node_name.toStdString().c_str());
    } else if (spa_streq(type, PW_TYPE_INTERFACE_Port)) {
        const uint32_t nodeId = pw_properties_get_uint32(global->properties, PW_KEY_NODE_ID, 0);
        if (!remote_data->ports->contains(nodeId))
            remote_data->ports->insert({nodeId, {{}, {}}});
        if (spa_streq(spa_dict_lookup(props, PW_KEY_PORT_DIRECTION), "in"))
            remote_data->ports->at(nodeId).first.push_back(id);
        else
            remote_data->ports->at(nodeId).second.push_back(id);
    } else if (spa_streq(type, PW_TYPE_INTERFACE_Link)) {
        std::pair key = {
            pw_properties_get_uint32(global->properties, PW_KEY_LINK_OUTPUT_NODE, 0),
            pw_properties_get_uint32(global->properties, PW_KEY_LINK_INPUT_NODE, 0)};
        if (!remote_data->links->contains(key))
            remote_data->links->insert({key, {}});
        remote_data->links->at(key).push_back(id);
    }
}

int destroy_global(void* obj, void* data) {
    auto* global = static_cast<struct::global*>(obj);

    if (!global)
        return -1;

    // if (global->proxy)
        // pw_proxy_destroy(global->proxy);
    pw_map_insert_at(&global->remote_data->globals, global->id, nullptr);
    if (spa_streq(global->type, PW_TYPE_INTERFACE_Node)) {
        global->remote_data->ports->erase(global->id);
    } else if (spa_streq(global->type, PW_TYPE_INTERFACE_Link)) {
        std::pair key = {
            pw_properties_get_uint32(global->properties, PW_KEY_LINK_OUTPUT_NODE, 0),
            pw_properties_get_uint32(global->properties, PW_KEY_LINK_INPUT_NODE, 0)};
        if (global->remote_data->links->at(key).size() <= 1)
            global->remote_data->links->erase(key);
        else
            global->remote_data->links->at(key).remove(global->id);
    }
    pw_properties_free(global->properties);
    free(global->type);
    delete global;
    return 0;
}

void registry_event_global_remove(void *data, uint32_t id) {
    auto* remote_data = static_cast<struct::remote_data*>(data);
    auto* global = static_cast<struct::global*>(pw_map_lookup(&remote_data->globals, id));
    if (!global) {
        fprintf(stderr, "removed unknown global %d\n", id);
        return;
    }

    if (global->modelItem) {
        if (remote_data->verbose)
            fprintf(stdout, "removed selectable node \"%s\"\n", global->modelItem->text().toStdString().c_str());
        QStandardItemModel* model = remote_data->instance->inputModel();
        remote_data->instance->main_window()->inputOptionRemoved(id);
        const int row = model->indexFromItem(global->modelItem).row();
        model->removeRow(row);
        global->modelItem = nullptr;
    }

    destroy_global(global, remote_data);
}

constexpr pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global,
    .global_remove = registry_event_global_remove
};

void on_core_done(void *_data, uint32_t id, int seq) {
    auto* remote_data = static_cast<struct::remote_data*>(_data);
    if (remote_data->cleanup) {
        remote_data->cleanup = false;
        if (remote_data->verbose)
            fprintf(stdout, "cleanup marking finished\n");
    }
}

void on_core_error(void *_data, uint32_t id, int seq, int res, const char *message) {
    auto* remote_data = static_cast<struct::remote_data*>(_data);

    pw_log_error("remote %p: error id:%u seq:%d res:%d (%d): %s", remote_data,
            id, seq, res, res, message);

    if (id == PW_ID_CORE && res == -EPIPE)
        remote_data->instance->quit();
}

constexpr pw_core_events remote_core_events = {
    .version = PW_VERSION_CORE_EVENTS,
    .done = on_core_done,
    .error = on_core_error
};

void on_core_destroy(void *_data) {
    auto* remote_data = static_cast<struct::remote_data*>(_data);
    struct data* data = remote_data->data;

    spa_hook_remove(&remote_data->core_listener);
    spa_hook_remove(&remote_data->proxy_core_listener);

    pw_map_for_each(&remote_data->globals, destroy_global, remote_data);
    pw_map_clear(&remote_data->globals);
    delete remote_data->ports;
    delete remote_data->links;

    if (data->remote_data == remote_data)
        data->remote_data = nullptr;
}

constexpr pw_proxy_events proxy_core_events = {
    .version = PW_VERSION_PROXY_EVENTS,
    .destroy = on_core_destroy
};

PipewireAPI::PipewireAPI() {
    data = nullptr;
    remote_data = nullptr;
    mainWindow = nullptr;
    inputs = new QStandardItemModel(0, 1);
}

PipewireAPI::~PipewireAPI() {
    quit();
    delete inputs;
}

void PipewireAPI::run(int argc, char *argv[]) {
    pw_init(&argc, &argv);

    data = new struct::data;
    data->thread_loop = pw_thread_loop_new("VAudio", nullptr);
    if (!data->thread_loop) {
        fprintf(stderr, "can't create main loop (broken installation)\n");
        return;
    }
    pw_thread_loop_lock(data->thread_loop);
    data->loop = pw_thread_loop_get_loop(data->thread_loop);

    data->context = pw_context_new(data->loop, nullptr, 0);
    if (!data->context) {
        pw_thread_loop_unlock(data->thread_loop);
        fprintf(stderr, "can't create context\n");
        return;
    }
    pw_context_load_module(data->context, "libpipewire-module-link-factory", nullptr, nullptr);

    struct pw_core* core = pw_context_connect(data->context, nullptr, sizeof(struct remote_data));
    if (!core) {
        pw_thread_loop_unlock(data->thread_loop);
        fprintf(stderr, "failed to connect\n");
        return;
    }
    remote_data = static_cast<struct::remote_data*>(pw_proxy_get_user_data(reinterpret_cast<struct::pw_proxy*>(core)));
    remote_data->instance = this;
    remote_data->core = core;
    remote_data->data = data;
    pw_map_init(&remote_data->globals, 64, 16);
    remote_data->ports = new std::map<uint32_t, std::pair<std::list<uint32_t>, std::list<uint32_t>>>;
    remote_data->links = new std::map<std::pair<uint32_t, uint32_t>, std::list<uint32_t>>;
    remote_data->cleanup = true;
    remote_data->verbose = argc >= 2 && spa_streq(argv[1], "--verbose");

    data->remote_data = remote_data;

    pw_core_add_listener(remote_data->core,
                   &remote_data->core_listener,
                   &remote_core_events, remote_data);
    pw_proxy_add_listener(reinterpret_cast<struct::pw_proxy*>(remote_data->core),
            &remote_data->proxy_core_listener,
            &proxy_core_events, remote_data);
    remote_data->registry = pw_core_get_registry(remote_data->core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(remote_data->registry,
                                    &remote_data->registry_listener,
                                    &registry_events, remote_data);
    pw_core_sync(remote_data->core, 0, 0);

    if (pw_thread_loop_start(data->thread_loop) < 0)
        fprintf(stderr, "failed to start thread loop\n");
    else if (remote_data->verbose)
        fprintf(stdout, "started pipewire thread\n");
    pw_thread_loop_unlock(data->thread_loop);
}

void PipewireAPI::quit() const {
    if (remote_data->verbose)
        fprintf(stdout, "stopping pipewire thread\n");
    pw_thread_loop_lock(data->thread_loop);
    pw_proxy_destroy(reinterpret_cast<struct::pw_proxy*>(remote_data->registry));
    pw_core_disconnect(remote_data->core);
    pw_context_destroy(data->context);
    pw_thread_loop_unlock(data->thread_loop);
    pw_thread_loop_stop(data->thread_loop);
    pw_thread_loop_destroy(data->thread_loop);
    pw_deinit();
    delete data;
}

std::string strToName(std::string string) {
    std::ranges::transform(string, string.begin(),
                           [](unsigned char c){ return c == ' ' ? '-' : std::tolower(c); });
    return string;
}

void PipewireAPI::loadSink(const std::string& name) const {
    if (remote_data->verbose)
        fprintf(stdout, "loading sink for %s:\n", name.c_str());
    system(("pactl load-module module-null-sink media.class=Audio/Sink channel_map=stereo sink_name=" + strToName(name) + R"( sink_properties="'node.description=\")" + name + R"(\" application.name=VAudio'")").c_str());
}

void PipewireAPI::loadSource(const std::string& name) const {
    if (remote_data->verbose)
        fprintf(stdout, "loading source for %s:\n", name.c_str());
    system(("pactl load-module module-null-sink media.class=Audio/Source/Virtual channel_map=stereo sink_name=" + strToName(name) + R"(-mic sink_properties="'node.description=\")" + name + R"(\" application.name=VAudio'")").c_str());
}

void PipewireAPI::unloadModule(const std::string& id) const {
    if (remote_data->verbose)
        fprintf(stdout, "removing sink/source with id %s\n", id.c_str());
    system(("pactl unload-module " + id).c_str());
}

const char* PipewireAPI::getId(const struct global* global) const {
    pw_thread_loop_lock(data->thread_loop);
    const char* id = spa_dict_lookup(
        static_cast<struct::pw_node_info*>(
            static_cast<struct::proxy_data*>(
                pw_proxy_get_user_data(global->proxy))->info)->props,
        "pulse.module.id");
    pw_thread_loop_unlock(data->thread_loop);
    return id;
}

void PipewireAPI::linkNodes(uint32_t outputId, uint32_t inputId) const {
    pw_thread_loop_lock(data->thread_loop);
    if (remote_data->links->contains({outputId, inputId})) {
        pw_thread_loop_unlock(data->thread_loop);
        fprintf(stderr, "node %u and %u have already been linked\n", outputId, inputId);
        return;
    }
    auto outPorts = remote_data->ports->at(outputId).second;
    auto inPorts = remote_data->ports->at(inputId).first;
    if (remote_data->verbose)
        fprintf(stdout, "linking node %u (%lu) and %u (%lu)\n",
            outputId, outPorts.size(), inputId, inPorts.size());
    struct pw_properties* props = pw_properties_new(nullptr, nullptr);
    pw_properties_set(props, PW_KEY_LINK_OUTPUT_NODE, std::to_string(outputId).c_str());
    pw_properties_set(props, PW_KEY_LINK_INPUT_NODE, std::to_string(inputId).c_str());
    if (outPorts.size() == inPorts.size()) {
        auto out = outPorts.begin();
        for (const uint32_t &inPort : inPorts) {
            pw_properties_set(props, PW_KEY_LINK_OUTPUT_PORT, std::to_string(*out).c_str());
            pw_properties_set(props, PW_KEY_LINK_INPUT_PORT, std::to_string(inPort).c_str());
            createLink(props);
            ++out;
        }
    } else if (outPorts.size() == 1) {
        for (const uint32_t &inPort : inPorts) {
            pw_properties_set(props, PW_KEY_LINK_OUTPUT_PORT, std::to_string(outPorts.front()).c_str());
            pw_properties_set(props, PW_KEY_LINK_INPUT_PORT, std::to_string(inPort).c_str());
            createLink(props);
        }
    } else if (inPorts.size() == 1) {
        for (const uint32_t &outPort : outPorts) {
            pw_properties_set(props, PW_KEY_LINK_OUTPUT_PORT, std::to_string(outPort).c_str());
            pw_properties_set(props, PW_KEY_LINK_INPUT_PORT, std::to_string(inPorts.front()).c_str());
            createLink(props);
        }
    }
    pw_properties_free(props);
    pw_thread_loop_unlock(data->thread_loop);
}

void PipewireAPI::unlinkNodes(uint32_t outputId, uint32_t inputId) const {
    pw_thread_loop_lock(data->thread_loop);
    if (!remote_data->links->contains({outputId, inputId})) {
        pw_thread_loop_unlock(data->thread_loop);
        fprintf(stderr, "node %u and %u are not linked\n", outputId, inputId);
        return;
    }
    if (remote_data->verbose)
        fprintf(stdout, "unlinking node %u and %u (%lu links)\n",
            outputId, inputId, remote_data->links->at({outputId, inputId}).size());
    for (uint32_t linkId : remote_data->links->at({outputId, inputId})) {
        pw_registry_destroy(remote_data->registry, linkId);
    }
    pw_thread_loop_unlock(data->thread_loop);
}

void PipewireAPI::setSinkVolume(const std::string& name, const int value) const {
    if (remote_data->verbose)
        fprintf(stdout, "setting sink \"%s\" volume to %u\n", name.c_str(), value);
    system(("pactl set-sink-volume " + name + " " + std::to_string(value) + "%").c_str());
}

std::string PipewireAPI::info_node(const struct proxy_data *pd) const {
    pw_thread_loop_lock(data->thread_loop);
    auto *info = static_cast<struct::pw_node_info*>(pd->info);

    std::string info_str = info_global(pd);
    if (!info) {
        pw_thread_loop_unlock(data->thread_loop);
        return "NULL";
    }
    if (info->change_mask & PW_NODE_CHANGE_MASK_INPUT_PORTS)
        info_str += "\tinput ports: " + std::to_string(info->n_input_ports) + "/" + std::to_string(info->max_input_ports) + "\n";
    if (info->change_mask & PW_NODE_CHANGE_MASK_OUTPUT_PORTS)
        info_str += "\toutput ports: " + std::to_string(info->n_output_ports) + "/" + std::to_string(info->max_output_ports) + "\n";
    if (info->change_mask & PW_NODE_CHANGE_MASK_STATE)
        info_str += "\tstate: \"" + std::string(pw_node_state_as_string(info->state)) + "\"";
    if (info->state == PW_NODE_STATE_ERROR && info->error)
        info_str += " \"" + std::string(info->error) + "\"\n";
    else
        info_str += "\n";
    if (info->change_mask & PW_NODE_CHANGE_MASK_PROPS)
        info_str += info_properties(info->props);
    pw_thread_loop_unlock(data->thread_loop);
    return info_str.substr(0, info_str.length() - 1);
}

std::string PipewireAPI::info_global(const struct proxy_data *pd) {
    const struct global *global = pd->global;

    if (!global)
        return "NULL";

    return "id: " + std::to_string(global->id) + "\ntype: " + global->type + "/" + std::to_string(global->version) + "\n";
}

std::string PipewireAPI::info_properties(const struct spa_dict *props) {
    const struct spa_dict_item *item;
    std::string info_str = "\tproperties:\n";

    if (!props || props->n_items == 0) {
        info_str += "\t\tnone\n";
        return info_str;
    }

    spa_dict_for_each(item, props) {
        info_str += std::string("\t\t") + item->key + " = \"" + item->value + "\"\n";
    }
    return info_str;
}

void PipewireAPI::createLink(const struct pw_properties *props) const {
    auto* proxy = static_cast<struct::pw_proxy*>(pw_core_create_object(
        remote_data->core,
        "link-factory",
        PW_TYPE_INTERFACE_Link,
        PW_VERSION_LINK,
        &props->dict,
        sizeof(struct proxy_data)));
    auto* proxy_data = static_cast<struct::proxy_data*>(pw_proxy_get_user_data(proxy));
    proxy_data->remote_data = remote_data;
    proxy_data->proxy = proxy;
    proxy_data->pw_class = &link_class;
    pw_proxy_add_object_listener(proxy, &proxy_data->object_listener, &link_events, proxy_data);
    pw_proxy_add_listener(proxy, &proxy_data->proxy_listener, &proxy_events, proxy_data);
}
