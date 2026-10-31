//
// Created by SuperTuxi on 11.06.25.
//

#include "addsinkpanel.h"

#include "mainwindow.h"

AddSinkPanel::AddSinkPanel(MainWindow* mainWindow) : confirmButton(QIcon(":/icons/emblem-ok"), "Confirm") {
    this->mainWindow = mainWindow;
    this->setFixedHeight(150);
    layout.setSpacing(4);
    nameBox.setPlaceholderText("Enter Sink Name");
    nameBox.setAlignment(Qt::AlignCenter);
    nameBox.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    confirmButton.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    layout.addWidget(&nameBox, 0, 0);
    layout.setRowMinimumHeight(0, 73);
    layout.addWidget(&confirmButton, 1, 0);
    layout.setRowMinimumHeight(1, 73);
    layout.setAlignment(&confirmButton, Qt::AlignCenter);
    this->setLayout(&layout);

    connect(&nameBox, &QLineEdit::returnPressed, this, &AddSinkPanel::onNameBoxReturnPressed);
    connect(&confirmButton, &QPushButton::clicked, this, &AddSinkPanel::onConfirmButtonClicked);
}

void AddSinkPanel::onNameBoxReturnPressed() {
    mainWindow->addSinkPanel(this, nameBox.text());
}

void AddSinkPanel::onConfirmButtonClicked() {
    mainWindow->addSinkPanel(this, nameBox.text());
}
