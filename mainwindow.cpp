//
// Created by SuperTuxi on 11.06.25.
//

#include "mainwindow.h"

#include <qcoreapplication.h>
#include <qevent.h>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>

#include "addsinkpanel.h"
#include "inputpanel.h"
#include "sinkpanel.h"

MainWindow::MainWindow(PipewireAPI* pipewire_api) : QSplitter(Qt::Horizontal), sinkArea(nullptr, 390),
                            addSinkButton(QIcon(":/icons/list-add"), ""),
                            openSinkButton(QIcon(":/icons/document-open"), ""),
                            inputArea(nullptr, 500),
                            noSinkSelectedLabel("No Sink selected\nSelect a Sink to edit the inputs"),
                            addInputButton(QIcon(":/icons/list-add"), ""),
                            trayIcon(QIcon(":/icons/audio-card"), this), trayIconMenu(this),
                            debugWindow(&inputArea) {
    this->pipewire_api = pipewire_api;
    pipewire_api->set_main_window(this);
    this->checkedSinkPanel = nullptr;
    this->setWindowIcon(QIcon(":/icons/audio-card"));
    this->setStyleSheet("* { font-family: \"Open Sans SemiBold\"; font-size: 16px; color: #EEEEEE }"
        "QSplitter { background-color: #191919; }"
        "QPushButton { background-color: #191919; border-radius: 3px; padding: 4px 3px; }"
        "QPushButton:hover { background-color: #313131; }"
        "QPushButton:pressed, QPushButton:checked { background-color: #101010; }"
        "QPushButton:disabled { background-color: #222222; color: #9B9B9B; }"
        "QPushButton#icon { padding: 4px 2px; }"
        "QPushButton#add { icon: url(':/icons/list-add'); padding: 4px 2px; }"
        "QPushButton#add:hover, QPushButton#add:pressed, QPushButton#add:checked { icon: url(':/icons/list-add_active'); }"
        "QPushButton#warning { icon: url(':/icons/dialog-warning'); padding: 4px 2px; }"
        "QPushButton#warning:hover, QPushButton#warning:pressed, QPushButton#warning:checked { icon: url(':/icons/dialog-warning_active'); }"
        "QPushButton#stop { icon: url(':/icons/process-stop'); }"
        "QPushButton#stop:hover, QPushButton#stop:pressed, QPushButton#stop:checked { icon: url(':/icons/process-stop_active'); }"
        "QPushButton#stop-icon { icon: url(':/icons/process-stop'); padding: 4px 2px; }"
        "QPushButton#stop-icon:hover, QPushButton#stop-icon:pressed, QPushButton#stop-icon:checked { icon: url(':/icons/process-stop_active'); }"
        "QPushButton#lock { icon: url(':/icons/changes-allow'); padding: 4px 2px; }"
        "QPushButton#lock:checked { icon: url(':/icons/changes-prevent'); }"
        "QPushButton#microphone { icon: url(':/icons/microphone-disabled'); }"
        "QPushButton#microphone:checked { icon: url(':/icons/audio-input-microphone'); }"
        "QPushButton#mute { icon: url(':/icons/audio-volume-high'); padding: 4px 2px; }"
        "QPushButton#mute:checked { icon: url(':/icons/audio-volume-muted'); }"
        "QPushButton#mute:disabled { icon: url(':/icons/audio-volume-unavailable'); }"
        "SinkPanel { background-color: #00000000; }"
        "QWidget#scroll { background-color: #262626; margin: 10px; border-radius: 25px; }"
        "QLabel#notice { font-size: 20px; color: #AFAFAF; }"
        "QLabel#title { font-size: 25px; }"
        "QLineEdit { background-color: #1E1E1E; font-size: 20px; }"
        "QComboBox { background-color: #1E1E1E; }");
    this->setChildrenCollapsible(false);
    this->resize(1280, 720);

    sinkArea.setObjectName("scroll");
    sinksWidget.setObjectName("scroll");
    sinksLayout.setSpacing(10);
    sinkAddOptions.setSpacing(4);
    sinkAddOptions.setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    sinkAddOptions.setContentsMargins(0, 10, 0, 10);
    addSinkButton.setObjectName("add");
    openSinkButton.setObjectName("icon");
    sinkAddOptions.addWidget(&addSinkButton);
    sinkAddOptions.addWidget(&openSinkButton);
    sinksLayout.addLayout(&sinkAddOptions);
    sinksWidget.setLayout(&sinksLayout);
    sinkArea.setWidget(&sinksWidget);

    inputArea.setObjectName("scroll");
    inputWidget.setObjectName("scroll");
    inputsLayout.setAlignment(Qt::AlignTop);
    noSinkSelectedLabel.setObjectName("notice");
    noSinkSelectedLabel.setAlignment(Qt::AlignCenter);
    noSinkSelectedLabel.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    inputAddOptions.setSpacing(4);
    inputAddOptions.setContentsMargins(0, 10, 0, 10);
    inputAddOptions.setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    addInputButton.setObjectName("add");
    addInputButton.hide();
    inputAddOptions.addWidget(&addInputButton);
    inputsLayout.addWidget(&noSinkSelectedLabel);
    inputsLayout.addLayout(&inputAddOptions);
    inputWidget.setLayout(&inputsLayout);
    inputArea.setWidget(&inputWidget);

    this->addWidget(&sinkArea);
    this->addWidget(&inputArea);
    int sinkAreaWidth = sinkArea.minimumSize().width();
    this->setSizes(QList({sinkAreaWidth, this->width() - sinkAreaWidth}));

    QAction* openAction = trayIconMenu.addAction("Open");
    QAction* quitAction = trayIconMenu.addAction("Quit");
    trayIcon.setContextMenu(&trayIconMenu);
    trayIcon.show();

    debugWindow.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    debugWindow.setMargin(10);

    connect(&addSinkButton, &QPushButton::clicked, this, &MainWindow::addSinkButtonClicked);
    connect(&openSinkButton, &QPushButton::clicked, this, &MainWindow::openSinkButtonClicked);
    connect(&trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);
    connect(openAction, &QAction::triggered, this, &MainWindow::show);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quit);
}

bool MainWindow::addSinkPanel(AddSinkPanel* addSinkPanel, const QString& name) {
    if (name.isEmpty() || name.isNull())
        return false;
    for (const auto& [nameKey, sink] : sinks) {
        if (sink->name() == name)
            return false;
    }
    auto* sinkPanel = new SinkPanel(this, name);
    if (!addSinkPanel) {
        sinksLayout.insertWidget(sinksLayout.count() - 1, sinkPanel);
    } else {
        sinksLayout.replaceWidget(addSinkPanel, sinkPanel);
        delete addSinkPanel;
    }
    sinks[sinkPanel->name().toStdString()] = sinkPanel;
    sinkPanel->load();
    addSinkButton.setDisabled(false);
    return true;
}

void MainWindow::setSinkPanelChecked(SinkPanel *sinkPanel, bool checked) {
    if (checked) {
        if (checkedSinkPanel)
            checkedSinkPanel->setChecked(false);
        checkedSinkPanel = sinkPanel;
        noSinkSelectedLabel.hide();
        addInputButton.show();
        connect(&addInputButton, &QPushButton::clicked, sinkPanel, &SinkPanel::addInputPanel);
    } else if (sinkPanel == checkedSinkPanel) {
        checkedSinkPanel = nullptr;
        noSinkSelectedLabel.show();
        addInputButton.hide();
        disconnect(&addInputButton, &QPushButton::clicked, sinkPanel, &SinkPanel::addInputPanel);
    }
}

void MainWindow::setSinkPanelSink(struct global *global) {
    sinks[spa_dict_lookup(&global->properties->dict, PW_KEY_NODE_DESCRIPTION)]->set_sink(global);
}

void MainWindow::setSinkPanelSource(struct global *global) {
    sinks[spa_dict_lookup(&global->properties->dict, PW_KEY_NODE_DESCRIPTION)]->set_source(global);
}

void MainWindow::removeSinkPanel(SinkPanel *sinkPanel) {
    setSinkPanelChecked(sinkPanel, false);
    sinksLayout.removeWidget(sinkPanel);
    sinks.erase(sinkPanel->name().toStdString());
    delete sinkPanel;
}

void MainWindow::addInputPanel(InputPanel* inputPanel) {
    inputsLayout.insertWidget(inputsLayout.count() - 1, inputPanel);
}

void MainWindow::inputOptionInserted(struct proxy_data* proxyData) {
    for (auto [name, sink] : sinks)
        sink->inputOptionInserted(proxyData);
}

void MainWindow::inputOptionRemoved(uint32_t id) {
    for (auto [name, sink] : sinks)
        sink->inputOptionRemoved(id);
}

void MainWindow::removeInputPanel(InputPanel* inputPanel) {
    inputsLayout.removeWidget(inputPanel);
    delete inputPanel;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    int sinkAreaWidth = this->sizes().at(0);
    QSplitter::resizeEvent(event);
    this->setSizes(QList({sinkAreaWidth, this->width() - sinkAreaWidth}));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    this->hide();
    event->ignore();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    this->setVisible(!this->isVisible());
}

void MainWindow::quit() {
    for (auto [name, sink] : sinks)
        sink->unload();
    delete pipewire_api;
    qApp->quit();
}

void MainWindow::show_debug(const QString& text) {
    debugWindow.setText(text);
    debugWindow.show();
    debugWindow.setFixedSize(debugWindow.minimumSizeHint());
}

void MainWindow::hide_debug() {
    debugWindow.setText("");
    debugWindow.setFixedSize(0, 0);
    debugWindow.hide();
}

void MainWindow::addSinkButtonClicked() {
    sinksLayout.insertWidget(sinksLayout.count() - 1, new AddSinkPanel(this));
    addSinkButton.setDisabled(true);
}

void MainWindow::openSinkButtonClicked() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Save Sink Configuration as File", QString(), "JSON Files (*.json)");
    for (const auto& fileName : files) {
        if (fileName.isEmpty() || fileName.isNull())
            continue;
        QFile loadFile(fileName);
        if (!loadFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open save file.");
            return;
        }
        QByteArray data = loadFile.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(data));
        if (const QJsonValue v = loadDoc["name"]; v.isString()) {
            QString name = v.toString();
            if (addSinkPanel(nullptr, name))
                sinks[name.toStdString()]->readJson(loadDoc.object());
        }
    }
}

