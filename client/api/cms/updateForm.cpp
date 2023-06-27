#include <newt.h>
#include <stdio.h>
#include <unistd.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>

QByteArray
openExternalEditor(const QByteArray& data) {
    // 暂停newt库，恢复原始终端状态
    newtSuspend();

    QByteArray newData = data;

    QTemporaryFile tmpFile(QDir::tempPath() + "/XXXXXX.tmp");

    if (tmpFile.open()) {
        tmpFile.write(data);
        tmpFile.close();

        // 调用外部程序，如vim
        system(
            QString("vim %1").arg(tmpFile.fileName()).toLatin1().constData());

        if (tmpFile.open()) {
            newData = tmpFile.readAll();
            newData.replace('\n', "\\n");
            tmpFile.close();
        }

        tmpFile.remove();
    }

    // 恢复newt库，重新显示对话框
    newtResume();
    newtRefresh();

    return newData;
}

int
main(int argc, char* argv[]) {
    if (!qEnvironmentVariableIsEmpty("INVIM")) return 1;

    QJsonDocument jsonDoc;

    bool isNew = false;
    if (argc > 1)
        jsonDoc = QJsonDocument::fromJson(argv[1]);
    else
        isNew = true;

    newtInit();
    newtCls();

    newtComponent form, nameLabel, nameEntry, titleLabel, titleEntry,
        contentTypeLabel, contentTypeEntry, seqLabel, seqEntry,
        /*hideLabel,*/ hideCheckbox,
        /*privateLabel,*/ privateCheckbox, /*extDataButton, summaryButton,*/
        typeLabel, fileRadioButton, dirRadioButton, refRadioButton, okButton,
        cancelButton;

    int rc = newtCenteredWindow(32, 17, isNew ? "New Node" : "Update Node");

    // clang-format on
    nameLabel = newtLabel(1, 1, "        Name:");
    nameEntry = newtEntry(14, 1, jsonDoc["name"].toString("").toLatin1(), 18,
                          NULL, NEWT_ENTRY_SCROLL);

    titleLabel = newtLabel(1, 2, "       Title:");
    titleEntry = newtEntry(14, 2, jsonDoc["title"].toString("").toUtf8(), 18,
                           NULL, NEWT_ENTRY_SCROLL);

    contentTypeLabel = newtLabel(1, 3, "Content Type:");
    QString contentTypeValue = jsonDoc["contentType"].toString("");
    contentTypeEntry = newtEntry(14, 3, contentTypeValue.toLatin1(), 18, NULL,
                                 NEWT_ENTRY_SCROLL);

    if (contentTypeValue == "ref")
        newtEntrySetFlags(contentTypeEntry, NEWT_FLAG_DISABLED, NEWT_FLAGS_SET);

    seqLabel = newtLabel(1, 4, "    Sequence:");
    seqEntry =
        newtEntry(14, 4, QString::number(jsonDoc["seq"].toInt(-1)).toLatin1(),
                  18, NULL, NEWT_ENTRY_SCROLL);

    // hideLabel = newtLabel(1, 4,        "        Hide:");
    hideCheckbox =
        newtCheckbox(14, 6, "Hide          ",
                     jsonDoc["hide"].toInt() > 0 ? 'X' : ' ', " X", NULL);

    // privateLabel = newtLabel(1, 5,     "     Private:");
    privateCheckbox =
        newtCheckbox(14, 7, "Private       ",
                     jsonDoc["private"].toInt() > 0 ? 'X' : ' ', " X", NULL);

    typeLabel = newtLabel(1, 9, "        Type:");
    fileRadioButton =
        newtRadiobutton(14, 9, "File          ",
                        isNew ? 1 : jsonDoc["type"].toString() == "F", NULL);
    if (!isNew)
        newtCheckboxSetFlags(fileRadioButton, NEWT_FLAG_DISABLED,
                             NEWT_FLAGS_SET);
    dirRadioButton =
        newtRadiobutton(14, 10, "Directory     ",
                        jsonDoc["type"].toString() == "D", fileRadioButton);
    if (!isNew)
        newtCheckboxSetFlags(dirRadioButton, NEWT_FLAG_DISABLED,
                             NEWT_FLAGS_SET);
    refRadioButton =
        newtRadiobutton(14, 11, "References    ",
                        jsonDoc["type"].toString() == "R", dirRadioButton);
    if (!isNew)
        newtCheckboxSetFlags(refRadioButton, NEWT_FLAG_DISABLED,
                             NEWT_FLAGS_SET);

    // clang-format on

    /*QByteArray extData = jsonDoc["extData"].toString().toLatin1();
    extDataButton = newtCompactButton(2, 8, " ExtData ");

    QByteArray summaryData = jsonDoc[" summary "].toString().toLatin1();
    summaryButton = newtCompactButton(17, 8, " Summary ");*/

    okButton = newtButton(7, 13, "OK");
    cancelButton = newtButton(15, 13, "Cancel");

    form = newtForm(NULL, NULL, 0);
    newtFormAddComponents(form, nameLabel, nameEntry, titleLabel, titleEntry,
                          contentTypeLabel, contentTypeEntry, seqLabel,
                          seqEntry, /*hideLabel,*/
                          hideCheckbox, /*privateLabel,*/ privateCheckbox,
                          typeLabel, fileRadioButton, dirRadioButton,
                          refRadioButton,
                          /*extDataButton,
summaryButton,*/ okButton, cancelButton, NULL);

    bool updated = false;

    /* while (1) {
         QJsonObject jsonObj = jsonDoc.object();
         newtComponent resultBtn = newtRunForm(form);
         if (resultBtn == okButton) {*/
    QJsonObject updatedJsonObj;

    if (newtRunForm(form) == okButton) {
        int updateCheck = 0;
        if (jsonDoc["name"].toString() != newtEntryGetValue(nameEntry)) {
            updatedJsonObj["name"] = newtEntryGetValue(nameEntry);
            updateCheck |= 0b000001;
        }

        if (jsonDoc["title"].toString() != newtEntryGetValue(titleEntry)) {
            updatedJsonObj["title"] = newtEntryGetValue(titleEntry);
            updateCheck |= 0b000010;
        }

        if (jsonDoc["contentType"].toString() !=
            newtEntryGetValue(contentTypeEntry)) {
            updatedJsonObj["contentType"] = newtEntryGetValue(contentTypeEntry);
            updateCheck |= 0b000100;
        }

        if (jsonDoc["hide"].toInt() !=
            (newtCheckboxGetValue(hideCheckbox) == 'X' ? 1 : 0)) {
            updatedJsonObj["hide"] = newtCheckboxGetValue(hideCheckbox) == 'X';
            updateCheck |= 0b001000;
        }

        if (jsonDoc["private"].toInt() !=
            (newtCheckboxGetValue(privateCheckbox) == 'X' ? 1 : 0)) {
            updatedJsonObj["private"] =
                newtCheckboxGetValue(privateCheckbox) == 'X';
            updateCheck |= 0b010000;
        }

        qlonglong updatedSeq =
            QByteArray(newtEntryGetValue(seqEntry)).toLongLong();
        if (jsonDoc["seq"].toInteger(-1) != updatedSeq) {
            updatedJsonObj["seq"] = updatedSeq;
            updateCheck |= 0b100000;
        }

        if (isNew) {
            newtComponent currentTypeRadioButton =
                newtRadioGetCurrent(fileRadioButton);
            if (currentTypeRadioButton == fileRadioButton)
                updatedJsonObj["type"] = "F";
            else if (currentTypeRadioButton == dirRadioButton)
                updatedJsonObj["type"] = "D";
            else if (currentTypeRadioButton == refRadioButton)
                updatedJsonObj["type"] = "R";
        } else
            updatedJsonObj["id"] = jsonDoc["id"].toInt();

        if (updateCheck && (updateCheck & 0b000001 || !isNew)) {
            jsonDoc.setObject(updatedJsonObj);
            updated = true;
        }

        // break;
    } /* else if (resultBtn == extDataButton) {
         extData = openExternalEditor(extData);
         jsonObj["extData"] = QString(extData);
         jsonDoc.setObject(jsonObj);
     } else if (resultBtn == summaryButton) {
         summaryData = openExternalEditor(summaryData);
         jsonObj["summary"] = QString(summaryData);
         jsonDoc.setObject(jsonObj);
     } else
         break;
 }*/

    newtFinished();
    newtFormDestroy(form);

    if (updated) fprintf(stderr, "%s", jsonDoc.toJson().constData());

    return 0;
}
