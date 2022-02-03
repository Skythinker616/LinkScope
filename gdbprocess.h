#ifndef GDBPROCESS_H
#define GDBPROCESS_H

#include <QObject>
#include <QProcess>
#include <qcoreapplication.h>
#include <qfile.h>
#include <qregexp.h>
#include <qdebug.h>
#include <QTime>

class GDBProcess : public QObject
{
    Q_OBJECT
public:
    explicit GDBProcess(QObject *parent = nullptr);
    ~GDBProcess();
    QString runCmd(const QString &cmd);
    void start();
    void stop();
    void connectToRemote(const QString &addr);
    void disconnectFromRemote();
    void setTempSymbolFileName(const QString &name);
    void loadSymbolFile(const QString &path);
    void unloadSymbolFile();
    void setDisplayList(QStringList &list);
    QString captureValueFromDisplay(const QString &rawDisplay,const QString &name);
    bool getDoubleFromDisplayValue(const QString &rawValue,double &result);
    void setVarValue(const QString &varFullName,double value);
    bool checkExpandableType(const QString &varFullName);
    QStringList getVarListFromRawOutput(const QString &rawVarList);
    void removeInnerSection(QString &raw,int offset);

private:
    QProcess *process;
    QString tempSymbolFileName;
};

#endif // GDBPROCESS_H
