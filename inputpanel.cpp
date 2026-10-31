//
// Created by SuperTuxi on 11.06.25.
//

#include "inputpanel.h"

#include <QEvent>
#include <QLineEdit>

#include "mainwindow.h"
#include "sinkpanel.h"

InputPanel::InputPanel(SinkPanel* sinkPanel, struct input_data* inputData) : inputInformation(QIcon(":/icons/help-about"), ""),
                                                muteButton(QIcon(":/icons/audio-volume-unavailable"), ""),
                                                reconnectionOptions("Reconnection Options"),
                                                removeButton(QIcon(":/icons/process-stop"), "") {
    this->sinkPanel = sinkPanel;
    this->inputData = inputData;
    inputData->panel = this;
    this->setMinimumHeight(145);
    this->setContentsMargins(40, 25, 40, 15);
    layout.setSpacing(15);
    inputLayout.setSpacing(4);
    inputOptions.setFixedHeight(40);
    inputOptions.setModel(sinkPanel->main_window()->pipewire()->inputModel());
    inputOptions.setCurrentIndex(-1);
    inputOptions.installEventFilter(this);
    inputInformation.setObjectName("icon");
    inputInformation.setCheckable(true);
    inputInformation.setDisabled(true);
    inputInformation.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    inputLayout.addWidget(&inputOptions);
    inputLayout.addWidget(&inputInformation);
    actionLayout.setSpacing(25);
    actionLayout.setContentsMargins(10, 0, 10, 0);
    muteButton.setObjectName("mute");
    muteButton.setCheckable(true);
    muteButton.setDisabled(true);
    muteButton.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    reconnectionOptions.setObjectName("menu");
    reconnectionOptions.setMenu(&reconnectionOptionsMenu);
    removeButton.setObjectName("stop-icon");
    removeButton.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    actionLayout.addWidget(&muteButton);
    actionLayout.addWidget(&reconnectionOptions);
    actionLayout.addWidget(&removeButton);
    layout.addLayout(&inputLayout);
    layout.addLayout(&actionLayout);
    this->setLayout(&layout);

    connect(sinkPanel, &SinkPanel::hideInputPanels, this, &InputPanel::hideInputPanel);
    connect(sinkPanel, &SinkPanel::removeInputPanels, this, &InputPanel::removeButtonClicked);
    connect(sinkPanel, &SinkPanel::inputLockChanged, this, &InputPanel::inputLockChanged);
    connect(&inputOptions, &QComboBox::currentIndexChanged, this, &InputPanel::inputOptionIndexChanged);
    connect(inputOptions.model(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &InputPanel::inputOptionAboutToBeRemoved);
    connect(&inputInformation, &QPushButton::toggled, this, &InputPanel::inputInformationToggled);
    connect(&muteButton, &QPushButton::toggled, this, &InputPanel::muteButtonToggled);
    connect(&reconnectionOptionsMenu, &QMenu::triggered, this, &InputPanel::reconnectionOptionTriggered);
    connect(&removeButton, &QPushButton::clicked, this, &InputPanel::removeButtonClicked);

    muteButton.setChecked(inputData->muted);
    if (inputData->global) {
        inputOptions.setCurrentIndex(inputData->global->modelItem->row());
    } else if (!inputData->name.isNull()) {
        inputOptions.setPlaceholderText("(inactive) " + inputData->name);
        for (const auto&[key, value] : inputData->reconnectionOptionMap) {
            loadReconnectionOption(key.c_str(), value.c_str());
        }
    }
}

void InputPanel::setInputOption(const QStandardItem* item) {
    inputOptions.setCurrentIndex(item->row());
}

void InputPanel::hideInputPanel() {
    sinkPanel->main_window()->removeInputPanel(this);
    input_data()->panel = nullptr;
}

void InputPanel::inputLockChanged(const bool locked) {
    const bool inactive = inputOptions.currentIndex() == -1;
    inputOptions.setDisabled(locked);
    reconnectionOptionsMenu.setDisabled(locked || inactive);
    removeButton.setDisabled(locked);
}

void InputPanel::inputOptionIndexChanged(const int index) {
    if (index == -1) {
        inputInformation.setDisabled(true);
        muteButton.setDisabled(true);
        reconnectionOptionsMenu.setDisabled(true);
        return;
    }
    inputInformation.setDisabled(false);
    muteButton.setDisabled(false);
    reconnectionOptionsMenu.setDisabled(sinkPanel->isLocked());
    struct global* oldGlobal = inputData->global;
    inputData->global = inputOptions.currentData().value<struct::global*>();
    if (!sinkPanel->setInputPanelOption(inputData, oldGlobal ? oldGlobal->id : 0)) {
        inputData->global = oldGlobal;
        inputOptions.setCurrentIndex(oldGlobal ? oldGlobal->modelItem->row() : -1);
        return;
    }
    inputOptions.setPlaceholderText("(inactive) " + inputOptions.currentText());
    inputData->name = inputOptions.currentText();
    auto* nodeInfo = static_cast<struct::pw_node_info*>(static_cast<struct::proxy_data*>(
        pw_proxy_get_user_data(inputData->global->proxy))->info);
    if (inputData->reconnectionOptionMap.empty()) {
        inputData->reconnectionOptionMap[PW_KEY_NODE_NAME] = spa_dict_lookup(nodeInfo->props, PW_KEY_NODE_NAME);
    }
    SinkPanel::updateReconnectionOptionMap(inputData, nodeInfo);
    loadReconnectionOptions(nodeInfo);
}

void InputPanel::inputOptionAboutToBeRemoved(const QModelIndex &parent, int start, int end) {
    if (inputOptions.currentIndex() == -1) {
        inputOptions.update();
        inputInformation.setDisabled(true);
        muteButton.setDisabled(true);
        reconnectionOptionsMenu.setDisabled(true);
        for (auto action : reconnectionOptionsMenu.actions()) {
            if (!action->isChecked()) {
                reconnectionOptionsMenu.removeAction(action);
            }
        }
    }
}

void InputPanel::inputInformationToggled(const bool checked) {
    if (inputOptions.currentData().isNull()) {
        inputInformation.setChecked(false);
        sinkPanel->main_window()->hide_debug();
        return;
    }
    if (checked) {
        sinkPanel->main_window()->show_debug(QString(
            sinkPanel->main_window()->pipewire()->info_node(static_cast<struct::proxy_data*>(
                pw_proxy_get_user_data(inputOptions.currentData().value<struct::global*>()->proxy))).c_str()));
    } else {
        sinkPanel->main_window()->hide_debug();
    }
}

void InputPanel::muteButtonToggled(bool checked) {
    inputData->muted = checked;
    sinkPanel->inputPanelMute(inputData);
}

void InputPanel::loadReconnectionOption(const char* key, const char* value) {
    QAction* action = reconnectionOptionsMenu.addAction(QString((std::string(key) + " (\"" + value + "\")").c_str()));
    action->setCheckable(true);
    QVariant actionData;
    actionData.setValue(std::pair(key, value));
    action->setData(actionData);
    if (inputData->reconnectionOptionMap.find(key) != inputData->reconnectionOptionMap.end()) {
        action->setChecked(true);
    }
}

void InputPanel::loadReconnectionOptions(const struct pw_node_info *nodeInfo) {
    const struct spa_dict_item* item;
    reconnectionOptionsMenu.clear();
    spa_dict_for_each(item, nodeInfo->props) {
        loadReconnectionOption(item->key, item->value);
    }
}

void InputPanel::reconnectionOptionTriggered(QAction *action) {
    if (action->isChecked()) {
        auto pair = action->data().value<std::pair<const char*, const char*>>();
        inputData->reconnectionOptionMap[pair.first] = pair.second;
    } else if (inputData->reconnectionOptionMap.size() > 1) {
        inputData->reconnectionOptionMap.erase(action->data().value<std::pair<const char*, const char*>>().first);
    } else {
        action->setChecked(true);
    }
}

void InputPanel::removeButtonClicked() {
    sinkPanel->removeInputPanel(this);
}

bool InputPanel::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        return true;
    }
    return false;
}
