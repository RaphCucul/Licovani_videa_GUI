#include "aboutprogram.h"
#include "ui_aboutprogram.h"

#include <QFile>
#include <QApplication>
#include <QTextStream>

AboutProgram::AboutProgram(QString i_version, QString i_language, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutProgram)
{
    ui->setupUi(this);
    ui->pvLabel->setText("<b>Release notes for version "+i_version+"</b>");
    QFile file(":/rl/releaseNotes/"+i_language+".txt");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        QString text = in.readAll().toLocal8Bit();
        file.close();
        ui->releaseNotes->setHtml(text);
    }
    else {
        ui->releaseNotes->setText(tr("Sorry, no release notes file found."));
    }
    this->setModal(false);
}

AboutProgram::~AboutProgram()
{
    delete ui;
}
