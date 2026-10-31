//
// Created by SuperTuxi on 11.06.25.
//

#include "sinkpanel.h"

#include <QMouseEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#include "addsinkpanel.h"
#include "inputpanel.h"
#include "mainwindow.h"

SinkPanel::SinkPanel(MainWindow* mainWindow, const QString& name) : label(name), volumeSlider(Qt::Horizontal),
                                                             microphoneButton(QIcon(":/icons/microphone-disabled"), "Microphone"),
                                                             lockButton(QIcon(":/icons/changes-allow"), ""),
                                                             suspendButton(QIcon(":/icons/dialog-warning"), ""),
                                                             unloadButton(QIcon(":/icons/process-stop"), "Unload") {
    this->mainWindow = mainWindow;
    this->sink = nullptr;
    this->source = nullptr;
    this->setFixedHeight(150);
    this->setCheckable(true);
    label.setObjectName("title");
    label.setAlignment(Qt::AlignCenter);
    volumeLayout.setContentsMargins(25, 0, 15, 0);
    volumeLayout.setSpacing(4);
    volumeSlider.setMinimum(0);
    volumeSlider.setMaximum(100);
    volumeSlider.setValue(100);
    volumeSlider.setTickInterval(10);
    volumeSlider.setTickPosition(QSlider::TicksBelow);
    volumeValueBox.setRange(0, 200);
    volumeValueBox.setValue(100);
    volumeValueBox.setSuffix("%");
    volumeValueBox.setFixedSize(65, 30);
    volumeValueBox.setAlignment(Qt::AlignHCenter);
    volumeLayout.addWidget(&volumeSlider);
    volumeLayout.addWidget(&volumeValueBox);
    actionLayout.setSpacing(4);
    microphoneButton.setObjectName("microphone");
    microphoneButton.setCheckable(true);
    lockButton.setObjectName("lock");
    lockButton.setCheckable(true);
    suspendButton.setObjectName("warning");
    suspendButton.setCheckable(true);
    unloadButton.setObjectName("stop");
    sinkActionLayout.addWidget(&lockButton);
    sinkActionLayout.addWidget(&suspendButton);
    sinkActionLayout.addWidget(&unloadButton);
    actionLayout.addWidget(&microphoneButton);
    actionLayout.setAlignment(&microphoneButton, Qt::AlignCenter);
    actionLayout.addLayout(&sinkActionLayout);
    actionLayout.setAlignment(&sinkActionLayout, Qt::AlignCenter);
    layout.addWidget(&label);
    layout.addLayout(&volumeLayout);
    layout.addLayout(&actionLayout);
    this->setLayout(&layout);

    connect(this, &QPushButton::toggled, this, &SinkPanel::sinkPanelToggled);
    connect(&volumeSlider, &QSlider::valueChanged, this, &SinkPanel::volumeChanged);
    connect(&volumeValueBox, &QSpinBox::valueChanged, this, &SinkPanel::volumeChanged);
    connect(&microphoneButton, &QPushButton::toggled, this, &SinkPanel::microphoneButtonToggled);
    connect(&lockButton, &QPushButton::toggled, this, &SinkPanel::lockButtonToggled);
    connect(&suspendButton, &QPushButton::toggled, this, &SinkPanel::suspendButtonToggled);
    connect(&unloadButton, &QPushButton::clicked, this, &SinkPanel::unloadButtonClicked);
}

void SinkPanel::addInputPanel() {
    if (this->isChecked()) {
        auto* input = new InputPanel(this, new struct::input_data);
        inputs.insert(inputs.end(), input->input_data());
        mainWindow->addInputPanel(input);
    }
}

void SinkPanel::removeInputPanel(InputPanel* inputPanel) {
    inputs.remove(inputPanel->input_data());
    if (inputPanel->input_data()->global) {
        activeInputs.erase(inputPanel->input_data()->global->id);
        if (!inputPanel->input_data()->muted && !this->isSuspended())
            mainWindow->pipewire()->unlinkNodes(inputPanel->input_data()->global->id, sink->id);
    }
    reconnectionInputs.remove(inputPanel->input_data());
    delete inputPanel->input_data();
    mainWindow->removeInputPanel(inputPanel);
}

bool SinkPanel::setInputPanelOption(struct input_data* inputData, uint32_t oldId) {
    if (inputData->global->id == sink->id ||
        (source && inputData->global->id == source->id) ||
        activeInputs.find(inputData->global->id) != activeInputs.end())
        return false;
    if (oldId != 0) {
        activeInputs.erase(oldId);
        if (!inputData->muted && !this->isSuspended())
            mainWindow->pipewire()->unlinkNodes(oldId, sink->id);
    }
    reconnectionInputs.remove(inputData);
    activeInputs[inputData->global->id] = inputData;
    if (!inputData->muted && !this->isSuspended())
        mainWindow->pipewire()->linkNodes(inputData->global->id, sink->id);
    return true;
}

void SinkPanel::inputPanelMute(struct input_data* inputData) {
    if (this->isSuspended())
        return;
    if (inputData->muted)
        mainWindow->pipewire()->unlinkNodes(inputData->global->id, sink->id);
    else
        mainWindow->pipewire()->linkNodes(inputData->global->id, sink->id);
}

void SinkPanel::setInputPanelInactive(struct input_data* inputData) {
    activeInputs.erase(inputData->global->id);
    reconnectionInputs.insert(reconnectionInputs.end(), inputData);
    inputData->global = nullptr;
}

bool reconnectionMatches(struct input_data* reconnectionInput, struct pw_node_info* info) {
    return std::all_of(
        reconnectionInput->reconnectionOptionMap.begin(),
        reconnectionInput->reconnectionOptionMap.end(),
        [info](const std::pair<const std::string, std::string>& reconnectionOption) {
            const char* value = spa_dict_lookup(info->props, reconnectionOption.first.c_str());
            return value && reconnectionOption.second == std::string(value);
        });
}

void SinkPanel::inputOptionInserted(struct proxy_data* proxyData) {
    auto* info = static_cast<struct::pw_node_info*>(proxyData->info);
    for (const auto reconnectionInput : reconnectionInputs) {
        if (reconnectionMatches(reconnectionInput, info)) {
            if (reconnectionInput->panel) {
                reconnectionInput->panel->setInputOption(proxyData->global->modelItem);
            } else {
                reconnectionInput->global = proxyData->global;
                if (!setInputPanelOption(reconnectionInput, 0)) {
                    reconnectionInput->global = nullptr;
                    return;
                }
                updateReconnectionOptionMap(reconnectionInput, info);
            }
            break;
        }
    }
}

void SinkPanel::inputOptionRemoved(uint32_t id) {
    auto entry = activeInputs.find(id);
    if (entry != activeInputs.end())
        setInputPanelInactive(entry->second);
}

void SinkPanel::load() {
    mainWindow->pipewire()->loadSink(name().toStdString());
}

void SinkPanel::unload() {
    if (sink) {
        mainWindow->pipewire()->unloadModule(mainWindow->pipewire()->getId(sink));
        sink = nullptr;
    }
    if (source) {
        mainWindow->pipewire()->unloadModule(mainWindow->pipewire()->getId(source));
        source = nullptr;
    }
}

QJsonObject SinkPanel::toJson() const {
    QJsonObject json;
    json["name"] = name();
    json["microphone"] = microphoneButton.isChecked();
    json["locked"] = lockButton.isChecked();
    json["suspended"] = suspendButton.isChecked();
    QJsonArray inputs;
    for (auto input : this->inputs) {
        QJsonObject inputJson;
        if (input->global)
            inputJson["selected"] = static_cast<int>(input->global->id);
        if (!input->name.isNull() && !input->name.isEmpty())
            inputJson["name"] = input->name;
        inputJson["muted"] = input->muted;
        QJsonObject reconnectionJson;
        for (const auto& reconnectionOption : input->reconnectionOptionMap) {
            reconnectionJson[reconnectionOption.first.c_str()] = reconnectionOption.second.c_str();
        }
        if (!reconnectionJson.isEmpty())
            inputJson["reconnection"] = reconnectionJson;
        inputs.append(inputJson);
    }
    json["inputs"] = inputs;
    return json;
}

void SinkPanel::readJson(const QJsonObject &json) {
    if (const QJsonValue v = json["microphone"]; v.isBool())
        microphoneButton.setChecked(v.toBool());
    if (const QJsonValue v = json["locked"]; v.isBool())
        lockButton.setChecked(v.toBool());
    if (const QJsonValue v = json["suspended"]; v.isBool())
        suspendButton.setChecked(v.toBool());
    if (const QJsonValue v = json["inputs"]; v.isArray()) {
        const QJsonArray inputs = v.toArray();
        for (auto inputValue : inputs) {
            if (!inputValue.isObject())
                continue;
            QJsonObject input = inputValue.toObject();
            // ReSharper disable once CppDFAMemoryLeak
            auto* inputData = new struct::input_data;
            if (const QJsonValue value = input["name"]; value.isString())
                inputData->name = value.toString();
            if (const QJsonValue value = input["muted"]; value.isBool())
                inputData->muted = value.toBool();
            if (const QJsonValue value = input["reconnection"]; value.isObject()) {
                const QJsonObject reconnection = value.toObject();
                for (const auto& reconnectionKey : reconnection.keys()) {
                    inputData->reconnectionOptionMap[reconnectionKey.toStdString()] = reconnection[reconnectionKey].toString().toStdString();
                }
                uint32_t selected = 0;
                if (const QJsonValue selectedValue = input["selected"]; selectedValue.isDouble())
                    selected = selectedValue.toInt();
                const QStandardItemModel* model = mainWindow->pipewire()->inputModel();
                struct pw_node_info* updateInfo = nullptr;
                for (int row = 0; row < model->rowCount(); ++row) {
                    auto* global = model->item(row)->data(Qt::UserRole).value<struct::global*>();
                    auto* proxyData = static_cast<struct::proxy_data*>(pw_proxy_get_user_data(global->proxy));
                    auto* info = static_cast<struct::pw_node_info*>(proxyData->info);
                    if (!proxyData->handled || activeInputs.find(global->id) != activeInputs.end() || (updateInfo && global->id != selected))
                        continue;
                    if (reconnectionMatches(inputData, info)) {
                        inputData->global = global;
                        updateInfo = info;
                        if (global->id == selected)
                            break;
                    }
                }
                if (updateInfo) {
                    updateReconnectionOptionMap(inputData, updateInfo);
                    activeInputs[inputData->global->id] = inputData;
                } else {
                    reconnectionInputs.insert(reconnectionInputs.end(), inputData);
                }
            }
            this->inputs.insert(this->inputs.end(), inputData);
        }
    }
}

void SinkPanel::updateReconnectionOptionMap(struct input_data* inputData, struct pw_node_info* nodeInfo) {
    for (auto iter = inputData->reconnectionOptionMap.begin(); iter != inputData->reconnectionOptionMap.end(); ++iter) {
        const char* value = spa_dict_lookup(nodeInfo->props, iter->first.c_str());
        if (value) {
            iter->second = value;
        } else {
            inputData->reconnectionOptionMap.erase(iter);
            --iter;
        }
    }
}

void SinkPanel::sinkPanelToggled(bool checked) {
    mainWindow->setSinkPanelChecked(this, checked);
    if (checked) {
        for (auto input : inputs) {
            mainWindow->addInputPanel(new InputPanel(this, input));
        }
    } else {
        emit hideInputPanels();
    }
}

void SinkPanel::volumeChanged(int volume) {
    if (volumeSlider.value() == volumeValueBox.value())
        return;
    volumeSlider.setValue(volume);
    volumeValueBox.setValue(volume);
    mainWindow->pipewire()->setSinkVolume(pw_properties_get(sink->properties, PW_KEY_NODE_NAME), volume);
}

void SinkPanel::microphoneButtonToggled(bool checked) {
    if (checked) {
        mainWindow->pipewire()->loadSource(name().toStdString());
    } else if (source) {
        mainWindow->pipewire()->unloadModule(mainWindow->pipewire()->getId(source));
        source = nullptr;
    }
}

void SinkPanel::lockButtonToggled(bool checked) {
    suspendButton.setDisabled(checked);
    unloadButton.setDisabled(checked);
    emit inputLockChanged(checked);
}

void SinkPanel::suspendButtonToggled(bool checked) {
    if (checked) {
        for (auto activeInput : activeInputs) {
            if (!activeInput.second->muted)
                mainWindow->pipewire()->unlinkNodes(activeInput.first, sink->id);
        }
        if (source)
            mainWindow->pipewire()->unlinkNodes(sink->id, source->id);
    } else {
        for (auto activeInput : activeInputs) {
            if (!activeInput.second->muted)
                mainWindow->pipewire()->linkNodes(activeInput.first, sink->id);
        }
        if (source)
            mainWindow->pipewire()->linkNodes(sink->id, source->id);
    }
}

void SinkPanel::unloadButtonClicked() {
    mainWindow->setSinkPanelChecked(this, false);
    emit removeInputPanels();
    unload();
    mainWindow->removeSinkPanel(this);
}

void SinkPanel::mouseReleaseEvent(QMouseEvent *event) {
    QPushButton::mouseReleaseEvent(event);
    if (event->button() == Qt::RightButton) {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Sink Configuration as File", name() + ".json", "JSON Files (*.json)");
        if (fileName.isEmpty() || fileName.isNull())
            return;
        QFile saveFile(fileName);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            qWarning("Couldn't open save file.");
            return;
        }
        saveFile.write(QJsonDocument(toJson()).toJson());
    }
}
