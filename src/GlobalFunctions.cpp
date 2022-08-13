/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

    This program is licensed under the Academic Public License
    (APL) v1.0; You can use, redistribute and/or modify it in
    non-commercial academic environments under the terms of the
    APL as published by the QBlade project; See the file 'LICENSE'
    for details; Commercial use requires a commercial license
    (contact info@qblade.org).

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

***********************************************************************/

#include "GlobalFunctions.h"

#include <QRegularExpression>
#include <QDir>
#include <QDate>
#include <QTime>
#include <QDebug>
#include <QFileDialog>
#include <QProcess>
#include <QUuid>

#include "Store.h"
#include "Params.h"
#include "MainFrame.h"
#include "Globals.h"
#include "GLWidget.h"
#include "QMessageBox"
#include "Serializer.h"
#include "XWidgets.h"
#include "src/ColorManager.h"
#include "src/ImportExport.h"

#include "QBEM/Polar360.h"
#include "FoilModule/Airfoil.h"
#include "PolarModule/Polar.h"
#include "QBEM/Blade.h"
#include "QTurbine/QTurbine.h"
#include "QSimulation/QSimulation.h"
#include "Windfield/WindField.h"
#include "StructModel/StrModel.h"
#include "BinaryProgressDialog.h"
#include "src/FoilModule/FoilModule.h"

double findAbsMinMax(QVector<float> *vector){

    float max = -10;
    for (int i=0;i<vector->size();i++) if (fabs(vector->at(i)) > max) max = fabs(vector->at(i));
    return max;
}

double findMin(QVector<float> *vector){

    float min = 10e10;
    for (int i=0;i<vector->size();i++) if ( vector->at(i) < min) min = vector->at(i);
    return min;
}


double findMax(QVector<float> *vector){

    float max = -10e10;
    for (int i=0;i<vector->size();i++) if ( vector->at(i) > max) max = vector->at(i);
    return max;
}

void removeColumnXf(Eigen::MatrixXf &matrix, unsigned int colToRemove)
{
    unsigned int numRows = matrix.rows();
    unsigned int numCols = matrix.cols()-1;

    if( colToRemove < numCols )
        matrix.block(0,colToRemove,numRows,numCols-colToRemove) = matrix.block(0,colToRemove+1,numRows,numCols-colToRemove);

    matrix.conservativeResize(numRows,numCols);
}

void removeRowXf(Eigen::MatrixXf& matrix, unsigned int rowToRemove)
{
    unsigned int numRows = matrix.rows()-1;
    unsigned int numCols = matrix.cols();

    if( rowToRemove < numRows )
        matrix.block(rowToRemove,0,numRows-rowToRemove,numCols) = matrix.block(rowToRemove+1,0,numRows-rowToRemove,numCols);

    matrix.conservativeResize(numRows,numCols);
}


hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if( max > 0.0 ) {
        out.s = (delta / max);                  // s
    } else {
        // r = g = b = 0                        // s = 0, v is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
        if( in.g >= max )
            out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
        else
            out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}

rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;
}

QString makeNameWithHigherNumber (QString oldName) {
    int number = 2;
    int position = oldName.lastIndexOf(QRegularExpression(" [(][0-9]+[)]$"));
    if (position >= 0) {  // read and truncate old number if existing
        number = oldName.mid(position+2, oldName.length()-position-3).toInt() + 1;
        oldName.truncate(position);
    }

    return QString(oldName + " (%1)").arg(number);
}

QString UnifyString(QString strong){
    strong.replace(" e","_e");
    strong.replace(" E","_E");
    strong.replace("e ","e_");
    strong.replace("E ","E_");
    strong.replace("\\- ","\\-_");
    strong.replace(" \\-","_\\-");
    strong.replace("\\+ ","\\+_");
    strong.replace(" \\+","_\\+");
    strong.replace("_","");
    strong.replace(",",".");

    return strong;
}

QString truncateQStringMiddle(QString strong, int maxlength){

    strong = strong.trimmed();

    // Early exit if no truncation necessary
    if (strong.size() <= maxlength) return strong;

    int numRightChars = ceil(double(maxlength/2.0)) - 2; // -2 to accommodate the ".."
    int numLeftChars = floor(double(maxlength/2.0)) - 1; // -1 to accommodate the "."

    return strong.left(numLeftChars)+"..."+strong.right(numRightChars);
}

QString UpdateLastDirName(QString fileName){

    fileName.replace("/",QDir::separator()).replace("\\",QDir::separator());

    int pos = fileName.lastIndexOf(QDir::separator());

    if(pos>0 && isGUI) g_mainFrame->m_LastDirName = fileName.left(pos);

    return fileName.left(pos);
}

QStringList FileContentToQStringList(QString filename, bool giveWarning){
    QStringList list;
    list.clear();
    QFile XFile(filename);
    if (!XFile.open(QIODevice::ReadOnly) && giveWarning)
    {
        QString strange = "Could not read the file\n"+filename;
        if (isGUI) QMessageBox::warning(g_mainFrame, "Warning", strange);
        else qDebug() << "Warning: Could not read the file:" <<filename;
        return list;
    }
    QTextStream in(&XFile);
    while (!in.atEnd()){
        list.append(in.readLine());
    }
    XFile.close();
    return list;
}

bool FindKeywordInFile(QString value, QStringList File){

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    for (int i=0; i<FileContents.size();i++){
        for (int j=0; j<FileContents.at(i).size();j++){
            if (value == FileContents.at(i).at(j)) {
                return true;
            }
        }
    }

    return false;

}

QString FindValueInFile(QString value, QStringList File, QString *error_msg, bool setmsg, bool *found){

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    for (int i=0; i<FileContents.size();i++){
        for (int j=1; j<FileContents.at(i).size();j++){
            if (value == FileContents.at(i).at(j)) {
                if (found) *found = true;
                return FileContents.at(i).at(0);
            }
        }
    }
    if (found) *found = false;
    if (setmsg && error_msg) error_msg->append("\nKeyword: " + value + " not found!");
    return QString("");
}

QStringList FindLineWithKeyword(QString value, QStringList File, QString *error_msg, bool setmsg, bool *found, bool replace){

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        if (replace){
            Line.replace(","," ");
            Line.replace(";"," ");
        }
        QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    for (int i=0; i<FileContents.size();i++){
        for (int j=0; j<FileContents.at(i).size();j++){
            if (value == FileContents.at(i).at(j)) {
                if (found) *found = true;
                return FileContents.at(i);
            }
        }
    }
    if (found) *found = false;
    if (setmsg && error_msg) error_msg->append("\n Variable " + value + " not found in file");
    return QStringList();
}


void WriteStreamToFile(QString FileName, QStringList FileStream){

    QFile file;
    QTextStream stream;

    FileName.replace("/",QDir::separator()).replace("\\",QDir::separator());

    QString pathName;
    int pos = FileName.lastIndexOf(QDir::separator());
    if (pos > 0) pathName = FileName.left(pos);



    QDir dir;
    if (pathName.size()) dir.mkpath(pathName);

    file.setFileName(FileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "...cant open "<<FileName;
        return;
    }
    stream.setDevice(&file);

    for (int i=0;i<FileStream.size();i++){
        stream << FileStream[i] << endl;
    }

    file.close();

}


Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> FindMatrixInFile(QString value, QStringList File, int rows, int cols, QString *error_msg, bool setmsg, bool *found){

    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> mat(rows,cols);

    mat.setZero(rows,cols);

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }
    for (int i=0; i<FileContents.size();i++){
        for (int j=0; j<FileContents.at(i).size();j++){
            if (value == FileContents.at(i).at(j) && j == 0) {
                if (i+1 < FileContents.size()){
                    for (int skip = i+1;skip<FileContents.size();skip++){ // skipping empty lines here

                        if (FileContents.at(skip).size()){

                            bool ok;
                            QString(FileContents.at(skip).at(0)).toDouble(&ok);

                            if (ok){                                    // only trying to convert lines that start with a number, thus ignoring comment lines

                                for (int k=0; k<rows; k++){
                                    int line = skip+k;

                                    if (line < FileContents.size() && cols <= FileContents.at(line).size()){
                                        for (int c=0; c<cols;c++){
                                            double num = QString(FileContents.at(line).at(c)).toDouble(&ok);
                                            if (!ok){
                                                if (found) *found = false;
                                                if (setmsg) error_msg->append("\n Matrix " + value + ": Must be "+QString().number(rows,'f',0)+"x"+QString().number(cols,'f',0));
                                                return mat;
                                            }
                                            mat(k,c) = num;
                                        }
                                    }
                                }
                                if (found) *found = true;
                                return mat;
                            }
                        }
                    }
                }
            }
        }
    }

    if (found) *found = false;
    if (setmsg) error_msg->append("\n Matrix " + value + ": Must be "+QString().number(rows,'f',0)+"x"+QString().number(cols,'f',0));
    return mat;
}

QList< QStringList > FindStringDataTable(QString value, QStringList File, int cols, QString *error_msg, bool setmsg, bool *found){

    //first value of the table must be numeric!!!!!

    QList< QStringList > table;

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }
    for (int i=0; i<FileContents.size();i++){
        for (int j=0; j<FileContents.at(i).size();j++){

            if (value == FileContents.at(i).at(j)) {
                if (i+1 < FileContents.size()){
                    for (int skip = i+1;skip<FileContents.size();skip++){ // skipping empty lines here

                        if (FileContents.at(skip).size()){

                            bool ok;
                            QString(FileContents.at(skip).at(0)).toDouble(&ok);

                            if (ok){                                    // start extracting the data now

                                int line = skip;

                                while(ok){

                                    QStringList row;

                                    if (line < FileContents.size() && cols <= FileContents.at(line).size()){
                                        for (int c=0; c<FileContents.at(line).size();c++){
                                            row.append(QString(FileContents.at(line).at(c)));
                                        }
                                    }

                                    if(row.size() >= cols){
                                        table.append(row);
                                        ok = true;
                                    }
                                    else{
                                        ok = false;
                                    }
                                    line++;
                                }

                                if (table.size()){
                                    if (found) *found = true;
                                    return table;
                                }

                            }
                        }
                    }
                }
            }
        }
    }

    if (found) *found = false;
    if (setmsg) error_msg->append("\n Data Table " + value + ": no data found! Must be "+QString().number(cols,'f',0)+" per data row!");
    return table;

}



QList< QList<double> > FindNumericDataTable(QString value, QStringList File, int cols, QString *error_msg, bool setmsg, bool *found){

    QList< QList<double> > table;

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }
    for (int i=0; i<FileContents.size();i++){
        for (int j=0; j<FileContents.at(i).size();j++){

            if (value == FileContents.at(i).at(j)) {
                if (i+1 < FileContents.size()){
                    for (int skip = i+1;skip<FileContents.size();skip++){ // skipping empty lines here

                        if (FileContents.at(skip).size()){

                            bool ok;
                            QString(FileContents.at(skip).at(0)).toDouble(&ok);

                            if (ok){                                    // start extracting the data now

                                int line = skip;

                                while(ok){

                                    QList<double> row;

                                    if (line < FileContents.size() && cols <= FileContents.at(line).size()){
                                        for (int c=0; c<cols;c++){
                                            double num = QString(FileContents.at(line).at(c)).toDouble(&ok);
                                            if (ok){
                                                row.append(num);
                                            }
                                        }
                                    }

                                    if(row.size() == cols){
                                        table.append(row);
                                        ok = true;
                                    }
                                    else{
                                        ok = false;
                                    }
                                    line++;
                                }

                                if (table.size()){
                                    if (found) *found = true;
                                    return table;
                                }

                            }
                        }
                    }
                }
            }
        }
    }

    if (found) *found = false;
    if (setmsg) error_msg->append("\n Data Table " + value + ": no data found! Must be "+QString().number(cols,'f',0)+" per data row!");
    return table;
}

QList< QList<double> > FindNumericValuesInFile(int minColCount, QStringList File, QString *error_msg, QString FileName){

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    QList <QList<double> > values;
    for (int i=0;i<FileContents.size();i++){
        bool valid = true;
        QList<double> row;
        for (int j=0;j<FileContents.at(i).size();j++){
            if (!ANY_NUMBER.match(FileContents.at(i).at(j)).hasMatch()){
                valid = false;
            }
        }
        if (valid && FileContents.at(i).size() >= minColCount){
            for (int j=0;j<FileContents.at(i).size();j++){
                row.append(FileContents.at(i).at(j).toDouble());
            }
            values.append(row);
        }
    }

    if (!values.size() && error_msg) error_msg->append("\nNo values found in "+FileName+"\n");

    return values;

}

QList< QStringList > FindDlcTableInFile(int colCount, QStringList File, bool allowAuto, QString *error_msg, QString FileName){

    QString autoString = "";
    if (allowAuto) autoString = "auto";

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        if (list.size() == colCount){
            bool valid = true;
            if (!list.at(0).size()) valid = false;
            if (!list.at(1).size()) valid = false;
            if (!list.at(2).size()) valid = false;
            if (!ANY_NUMBER.match(list.at(3)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(4)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(5)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(6)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(7)).hasMatch()) valid = false;
            if (!list.at(8).size()) valid = false;
            if (!ANY_NUMBER.match(list.at(9)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(10)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(11)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(12)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(13)).hasMatch()) valid = false;
            if (!list.at(14).size()) valid = false;
            if (!ANY_NUMBER.match(list.at(15)).hasMatch() && list.at(15) != autoString) valid = false;
            if (!ANY_NUMBER.match(list.at(16)).hasMatch() && list.at(16) != autoString) valid = false;
            if (!ANY_NUMBER.match(list.at(17)).hasMatch() && list.at(17) != autoString) valid = false;
            if (!ANY_NUMBER.match(list.at(18)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(19)).hasMatch() && list.at(19) != autoString) valid = false;
            if (!ANY_NUMBER.match(list.at(20)).hasMatch() && list.at(20) != autoString) valid = false;
            if (!ANY_NUMBER.match(list.at(21)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(22)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(23)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(24)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(25)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(26)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(27)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(28)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(29)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(30)).hasMatch()) valid = false;
            if (!ANY_NUMBER.match(list.at(31)).hasMatch()) valid = false;

            if (valid) FileContents.append(list);
        }
    }

    if (!FileContents.size() && error_msg) error_msg->append("\nNo values found in "+FileName+"\n");

    return FileContents;

}

void emitObjectListsChanged(bool searchForLastActive){

    g_QSimulationStore.emitObjectListChanged(searchForLastActive);
    g_FlapStore.emitObjectListChanged(searchForLastActive);
    g_BDamageStore.emitObjectListChanged(searchForLastActive);
    g_DynPolarSetStore.emitObjectListChanged(searchForLastActive);
    g_360PolarStore.emitObjectListChanged(searchForLastActive);
    g_polarStore.emitObjectListChanged(searchForLastActive);
    g_foilStore.emitObjectListChanged(searchForLastActive);
    g_StrutStore.emitObjectListChanged(searchForLastActive);
    g_tdataStore.emitObjectListChanged(searchForLastActive);
    g_rotorStore.emitObjectListChanged(searchForLastActive);
    g_bemdataStore.emitObjectListChanged(searchForLastActive);
    g_tbemdataStore.emitObjectListChanged(searchForLastActive);
    g_cbemdataStore.emitObjectListChanged(searchForLastActive);
    g_propbemdataStore.emitObjectListChanged(searchForLastActive);
    g_propcbemdataStore.emitObjectListChanged(searchForLastActive);
    g_verticalRotorStore.emitObjectListChanged(searchForLastActive);
    g_dmsdataStore.emitObjectListChanged(searchForLastActive);
    g_tdmsdataStore.emitObjectListChanged(searchForLastActive);
    g_cdmsdataStore.emitObjectListChanged(searchForLastActive);
    g_verttdataStore.emitObjectListChanged(searchForLastActive);
    g_windFieldStore.emitObjectListChanged(searchForLastActive);
    g_bladeStructureStore.emitObjectListChanged(searchForLastActive);
    g_bladestructureloadingStore.emitObjectListChanged(searchForLastActive);
    g_noiseSimulationStore.emitObjectListChanged(searchForLastActive);
    g_QTurbinePrototypeStore.emitObjectListChanged(searchForLastActive);
    g_QVelocityCutPlaneStore.emitObjectListChanged(searchForLastActive);
    g_QTurbineSimulationStore.emitObjectListChanged(searchForLastActive);
    g_StrModelMultiStore.emitObjectListChanged(searchForLastActive);
    g_WaveStore.emitObjectListChanged(searchForLastActive);
    g_operationalPointStore.emitObjectListChanged(searchForLastActive);

}

void disableAllStoreSignals(){

    g_FlapStore.disableSignal();
    g_BDamageStore.disableSignal();
    g_DynPolarSetStore.disableSignal();
    g_foilStore.disableSignal();
    g_polarStore.disableSignal();
    g_360PolarStore.disableSignal();
    g_StrutStore.disableSignal();
    g_tdataStore.disableSignal();
    g_rotorStore.disableSignal();
    g_bemdataStore.disableSignal();
    g_tbemdataStore.disableSignal();
    g_cbemdataStore.disableSignal();
    g_propbemdataStore.disableSignal();
    g_propcbemdataStore.disableSignal();
    g_verticalRotorStore.disableSignal();
    g_dmsdataStore.disableSignal();
    g_tdmsdataStore.disableSignal();
    g_cdmsdataStore.disableSignal();
    g_verttdataStore.disableSignal();
    g_windFieldStore.disableSignal();
    g_bladeStructureStore.disableSignal();
    g_bladestructureloadingStore.disableSignal();
    g_noiseSimulationStore.disableSignal();
    g_QTurbinePrototypeStore.disableSignal();
    g_QSimulationStore.disableSignal();
    g_QVelocityCutPlaneStore.disableSignal();
    g_QTurbineSimulationStore.disableSignal();
    g_StrModelMultiStore.disableSignal();
    g_WaveStore.disableSignal();
    g_operationalPointStore.disableSignal();

}

void enableAllStoreSignals(){

    g_FlapStore.enableSignal();
    g_BDamageStore.enableSignal();
    g_DynPolarSetStore.enableSignal();
    g_foilStore.enableSignal();
    g_polarStore.enableSignal();
    g_360PolarStore.enableSignal();
    g_StrutStore.enableSignal();
    g_tdataStore.enableSignal();
    g_rotorStore.enableSignal();
    g_bemdataStore.enableSignal();
    g_tbemdataStore.enableSignal();
    g_cbemdataStore.enableSignal();
    g_propbemdataStore.enableSignal();
    g_propcbemdataStore.enableSignal();
    g_verticalRotorStore.enableSignal();
    g_dmsdataStore.enableSignal();
    g_tdmsdataStore.enableSignal();
    g_cdmsdataStore.enableSignal();
    g_verttdataStore.enableSignal();
    g_windFieldStore.enableSignal();
    g_bladeStructureStore.enableSignal();
    g_bladestructureloadingStore.enableSignal();
    g_noiseSimulationStore.enableSignal();
    g_QTurbinePrototypeStore.enableSignal();
    g_QSimulationStore.enableSignal();
    g_QVelocityCutPlaneStore.enableSignal();
    g_QTurbineSimulationStore.enableSignal();
    g_StrModelMultiStore.enableSignal();
    g_WaveStore.enableSignal();
    g_operationalPointStore.enableSignal();

}

void sortAllStores(){

    g_foilStore.sortStore();
    g_polarStore.sortStore();
    g_360PolarStore.sortStore();
    g_rotorStore.sortStore();
    g_bemdataStore.sortStore();
    g_tbemdataStore.sortStore();
    g_cbemdataStore.sortStore();
    g_propbemdataStore.sortStore();
    g_propcbemdataStore.sortStore();
    g_tdataStore.sortStore();
    g_verticalRotorStore.sortStore();
    g_dmsdataStore.sortStore();
    g_tdmsdataStore.sortStore();
    g_cdmsdataStore.sortStore();
    g_verttdataStore.sortStore();
    g_windFieldStore.sortStore();
    g_bladeStructureStore.sortStore();
    g_bladestructureloadingStore.sortStore();
    g_noiseSimulationStore.sortStore();
    g_StrutStore.sortStore();
    g_DynPolarSetStore.sortStore();
    g_FlapStore.sortStore();
    g_BDamageStore.sortStore();
    g_QTurbinePrototypeStore.sortStore();
    g_QSimulationStore.sortStore();
    g_QTurbineSimulationStore.sortStore();
    g_StrModelMultiStore.sortStore();
    g_QVelocityCutPlaneStore.sortStore();
    g_WaveStore.sortStore();
    g_operationalPointStore.sortStore();

}

void unloadAllControllers(){

    for (int i=0;i<g_QSimulationStore.size();i++){
        g_QSimulationStore.at(i)->unloadControllers();
    }
}

void clearAllStores(){

    if (debugStores) printStoreState();

    unloadAllControllers();

    g_FlapStore.clear();
    g_BDamageStore.clear();
    g_DynPolarSetStore.clear();
    g_operationalPointStore.clear();
    g_foilStore.clear();
    g_polarStore.clear();
    g_360PolarStore.clear();
    g_StrutStore.clear();
    g_tdataStore.clear();
    g_rotorStore.clear();
    g_bemdataStore.clear();
    g_tbemdataStore.clear();
    g_cbemdataStore.clear();
    g_propbemdataStore.clear();
    g_propcbemdataStore.clear();
    g_verticalRotorStore.clear();
    g_dmsdataStore.clear();
    g_tdmsdataStore.clear();
    g_cdmsdataStore.clear();
    g_verttdataStore.clear();
    g_windFieldStore.clear();
    g_bladeStructureStore.clear();
    g_bladestructureloadingStore.clear();
    g_noiseSimulationStore.clear();
    g_QTurbinePrototypeStore.clear();
    g_QSimulationStore.clear();
    g_QVelocityCutPlaneStore.clear();
    g_QTurbineSimulationStore.clear();
    g_StrModelMultiStore.clear();
    g_WaveStore.clear();

}

QString SerializeQBladeProject(QDataStream &ar, bool isStoring, QString ident, bool publicFormat, bool echo_off) {

    QString message;


    g_serializer.setDataStream(&ar);
    if (isStoring) {
        g_serializer.setMode(Serializer::WRITE);
        g_serializer.setArchiveFormat(VERSIONNUMBER);  //
        g_serializer.writeInt(g_serializer.getArchiveFormat()); // * 310005 : added propeller data
        g_serializer.writeInt(11229944);
        g_serializer.readOrWriteBool(&uintRes);
        g_serializer.readOrWriteBool(&uintVortexWake);
        // 310004 : added offshore dlc data
        // 310003 : added wind field shift
        // 310002 : added custom spectrum for waves
        // 310001 : added equal frequency discretization for waves
        // 310000 : QPR version of QBlade 2.0
        qDebug().noquote() <<"...storing QBlade Community Edition (CE) project file with versionnumber:" << g_serializer.getArchiveFormat() << "; Compressed Data Format:" << uintRes << "; Compressed Wake:" << uintVortexWake;
    } else {
        g_serializer.setMode(Serializer::READ);
        int format = g_serializer.readInt();
        g_serializer.setArchiveFormat(format);

        int check;
        g_serializer.readOrWriteInt(&check);

        if (check != 11229944 && check != 22339944)  // used to determine if the file actually is a QBlade file
        {
            throw Serializer::Exception ("The loaded file is not a QBlade file!");
        }

        if (check == 22339944) throw Serializer::Exception ("The loaded file is a QBlade Enterprise Edition (EE) File and cannot be loaded with the Community Edition (CE) version!");

        if (g_serializer.getArchiveFormat() > VERSIONNUMBER) {
            throw Serializer::Exception ("The loaded file uses a newer archive version! "
                                        "\nFile version: "+QString().number(g_serializer.getArchiveFormat(),'f',0)+
                                        "\nThis version: "+QString().number(VERSIONNUMBER,'f',0)+
                                         "\nPlease get the latest release of QBlade...");
        }

        if (g_serializer.getArchiveFormat() < COMPATIBILITY) {
            throw Serializer::Exception ("This file was saved in a prerelease version of QBlade, between v0.96b and v2.0, it needs to be converted!");
        }

        g_serializer.readOrWriteBool(&uintRes);
        g_serializer.readOrWriteBool(&uintVortexWake);

        qDebug().noquote() <<"...opening QBlade Community Edition (CE) project file with versionnumber:" << format << "; Compressed Data Format:" << uintRes << "; Compressed Wake" << uintVortexWake;
    }

    serializeAllStoresPublic(ident);

    if (g_serializer.isReadMode()) {
        g_serializer.restoreAllPointers();
        g_serializer.initializeAllPointers();
        sortAllStores();
    }

    g_serializer.setDataStream(NULL);

    uintRes = false;
    uintVortexWake = false;

    if (debugStores) printStoreState();

    return message;

}

void serializeAllStoresPublic(QString ident){

    int dummy = 0;

    const int compVersion = COMPATIBILITY;


    /* read or write the store contents here */
                                                if (debugStores) qDebug() << "serializing g_foilStore";
    if (ident.size() && g_serializer.isReadMode()){
        int n = g_foilStore.size();         // this allows to add an identifier to the lowest level foil objects when projects are merged
        g_serializer.readOrWriteInt(&n);
        for (int i=0;i<n;i++){
            Airfoil *foil;
            foil = Airfoil::newBySerialize();
            foil->setName(foil->getName()+ident);
            g_foilStore.add(foil,-1,true);
        }
    }
    else g_foilStore.serializeContent();                if (debugStores) qDebug() << "Store: serializing g_polarStore";
    g_polarStore.serializeContent();                    if (debugStores) qDebug() << "Store: serializing g_360PolarStore";
    g_360PolarStore.serializeContent();                 if (debugStores) qDebug() << "Store: serializing g_rotorStore";

    // we dont serialize the following stores from "old" QBlade projects to maintain compatibility
    if (g_serializer.getArchiveFormat() >= compVersion){
        g_rotorStore.serializeContent();                    if (debugStores) qDebug() << "Store: serializing g_bemdataStore";
        g_bemdataStore.serializeContent();                  if (debugStores) qDebug() << "Store: serializing g_tbemdataStore";
        g_tbemdataStore.serializeContent();                 if (debugStores) qDebug() << "Store: serializing g_cbemdataStore";
        g_cbemdataStore.serializeContent();                 if (debugStores) qDebug() << "Store: serializing g_tdataStore";
        g_tdataStore.serializeContent();                    if (debugStores) qDebug() << "Store: serializing g_verticalRotorStore";
        g_verticalRotorStore.serializeContent();            if (debugStores) qDebug() << "Store: serializing g_dmsdataStore";
        g_dmsdataStore.serializeContent();                  if (debugStores) qDebug() << "Store: serializing g_tdmsdataStore";
        g_tdmsdataStore.serializeContent();                 if (debugStores) qDebug() << "Store: serializing g_cdmsdataStore";
        g_cdmsdataStore.serializeContent();                 if (debugStores) qDebug() << "Store: serializing g_verttdataStore";
        g_verttdataStore.serializeContent();                if (debugStores) qDebug() << "Store: serializing g_windFieldStore";
        g_windFieldStore.serializeContent();                if (debugStores) qDebug() << "Store: serializing g_bladeStructureStore";
        g_bladeStructureStore.serializeContent();           if (debugStores) qDebug() << "Store: serializing g_bladestructureloadingStore";
        g_bladestructureloadingStore.serializeContent();    if (debugStores) qDebug() << "Store: serializing g_noiseSimulationStore";
        g_noiseSimulationStore.serializeContent();          if (debugStores) qDebug() << "Store: serializing g_StrutStore";
        g_StrutStore.serializeContent();                    if (debugStores) qDebug() << "Store: serializing g_DynPolarSetStore";
        g_DynPolarSetStore.serializeContent();              if (debugStores) qDebug() << "Store: serializing g_FlapStore";
        g_FlapStore.serializeContent();                     if (debugStores) qDebug() << "Store: serializing g_QTurbinePrototypeStore";
        g_QTurbinePrototypeStore.serializeContent();        if (debugStores) qDebug() << "Store: serializing g_QSimulationStore";
        g_QSimulationStore.serializeContent();              if (debugStores) qDebug() << "Store: serializing g_QTurbineSimulationStore";
        g_QTurbineSimulationStore.serializeContent();       if (debugStores) qDebug() << "Store: serializing g_StrModelMultiStore";
        g_StrModelMultiStore.serializeContent();            if (debugStores) qDebug() << "Store: serializing g_PlaneStore";
        g_serializer.readOrWriteInt(&dummy);                if (debugStores) qDebug() << "Store: serializing g_planeWingsStore";
        g_serializer.readOrWriteInt(&dummy);                if (debugStores) qDebug() << "Store: serializing g_DummyStore";
        g_serializer.readOrWriteInt(&dummy);                if (debugStores) qDebug() << "Store: serializing g_flightSimulationStore";
        g_serializer.readOrWriteInt(&dummy);                if (debugStores) qDebug() << "Store: serializing g_QVelocityCutPlaneStore";
        g_QVelocityCutPlaneStore.serializeContent();        if (debugStores) qDebug() << "Store: serializing g_DLCStore";
        g_serializer.readOrWriteInt(&dummy);                if (debugStores) qDebug() << "Store: serializing g_WaveStore";
        g_WaveStore.serializeContent();                     if (debugStores) qDebug() << "Store: serializing g_BDamageStore";
        g_BDamageStore.serializeContent();                  if (debugStores) qDebug() << "Store: serializing g_OperationalPointStore";
        g_operationalPointStore.serializeContent();
        if (g_serializer.getArchiveFormat() >= 310005){     if (debugStores) qDebug() << "Store: serializing g_propbemdataStore";
            g_propbemdataStore.serializeContent();          if (debugStores) qDebug() << "Store: serializing g_propcbemdataStore";
            g_propcbemdataStore.serializeContent();
        }
    }
}



void ExtractWpdataFileFromStream(QStringList &parameterStream, QStringList &wpDataStream, QString &wpDataFileName){

    //this is only needed to maintain backwards compatibility

    for (int i=0;i<parameterStream.size();i++){
        if (parameterStream.at(i).contains("WPDATATOSTREAM")){

            QString line = parameterStream.at(i);
            QStringList content = line.split(":");
            if (content.size() < 1) return;
            wpDataFileName = content.at(1);

            if (i+1 < parameterStream.size()){
                for (int k=i+1;k<parameterStream.size();k++){
                    wpDataStream.append(parameterStream.at(k));
                }
            }
        }
    }

    for (int i=0;i<parameterStream.size();i++){
        if (parameterStream.at(i).contains("WPDATATOSTREAM")){
            for (int j = parameterStream.size()-1; j>=i;j--){
                parameterStream.removeAt(j);
            }
            break;
        }
    }
}

void ReadFileToStream(QString &fileName, QStringList &stream, bool updateLastDir){

    fileName.replace("/",QDir::separator()).replace("\\",QDir::separator());
    stream.clear();

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open File", g_mainFrame->m_LastDirName,"File (*.*)");

    if (updateLastDir) UpdateLastDirName(fileName);

    QFile File(fileName);
    if (!File.open(QIODevice::ReadOnly))
    {
        return;
    }
    QTextStream in(&File);

    while(!in.atEnd())
    {
        stream.append(in.readLine());
    }

    File.close();

    if (fileName.size()){
        fileName.replace("/",QDir::separator()).replace("\\",QDir::separator());
        int pos = fileName.size() - 1 - fileName.lastIndexOf(QDir::separator());
        if (pos > -1) fileName = fileName.right(pos);
    }

}

bool ReadControllerParameterFile(QString &fileName, QString &wpDataFileName, QStringList &parameterStream, QStringList &wpDataStream, int controllerType){

    fileName.replace("/",QDir::separator()).replace("\\",QDir::separator());
    parameterStream.clear();
    wpDataStream.clear();
    wpDataFileName.clear();

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Controller Parameter File", g_mainFrame->m_LastDirName,"Controller Parameter File (*.*)");

    QString pathName;
    if (fileName.size()){
        fileName.replace("/",QDir::separator()).replace("\\",QDir::separator());
        int pos = fileName.lastIndexOf(QDir::separator());
        if(pos>0 && isGUI) g_mainFrame->m_LastDirName = fileName.left(pos);
        pathName =  fileName.left(pos);
    }

    QFile File(fileName);
    if (!File.open(QIODevice::ReadOnly))
    {
        QString strange = "Could not read the file\n"+fileName;
        if (isGUI) QMessageBox::warning(g_mainFrame, "Warning", strange);
        else qDebug() << "Could not read the file\n"+fileName;
        return false;
    }
    QTextStream in(&File);

    while(!in.atEnd())
    {
        parameterStream.append(in.readLine());
    }

    if (controllerType == DTU){
        QStringList parameterStream = FileContentToQStringList(fileName);

        int wp = 0;
        int n = 5;
        QString value = "constant";
        bool found = false;

        QList<QStringList> FileContents;
        for (int i=0;i<parameterStream.size();i++)
        {
            QString Line = QString(parameterStream.at(i)).simplified();
            QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
            FileContents.append(list);
        }

        for (int i=1; i<FileContents.size();i++){
            if (!found){
                for (int j=0; j<FileContents.at(i).size();j++){
                    if (value == FileContents.at(i).at(j)) {
                        if (j+1 < FileContents.at(i).size()){
                            if (n == QString(FileContents.at(i).at(j+1)).toInt()){
                                if (j+2 < FileContents.at(i).size()){

                                    double param = QString(FileContents.at(i).at(j+2)).toDouble(&found);

                                    if (found){
                                        wp = param;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (wp > 90){
            wpDataFileName = "wpdata."+QString().number(wp,'f',0);
            QFile wpdata(pathName+QDir::separator()+wpDataFileName);
            if (!wpdata.exists()){
                QString strange = "Could not find the wpdata file: "+wpdata.fileName();
                if (isGUI) QMessageBox::warning(g_mainFrame, "Warning", strange);
                else qDebug() << "Warning Could not find the wpdata file: "+wpdata.fileName();
                parameterStream.clear();
                wpDataStream.clear();
                return false;
            }
            else{
                if (!wpdata.open(QIODevice::ReadOnly))
                {
                    QString strange = "Could not find the wpdata file: "+wpdata.fileName();
                    if (isGUI) QMessageBox::warning(g_mainFrame, "Warning", strange);
                    else qDebug() << "Warning: Could not find the wpdata file: "+wpdata.fileName();
                    parameterStream.clear();
                    wpDataStream.clear();
                    return false;
                }
                QTextStream in2(&wpdata);
                while(!in2.atEnd())
                {
                    wpDataStream.append(in2.readLine());
                }
            }
        }
    }

    int pos = fileName.lastIndexOf(QDir::separator());
    pos = fileName.size()-pos-1;
    fileName = fileName.right(pos);

    File.close();

    return true;
}

bool ReadSimOrMotionFileFromStream(QString &fileName, QStringList &fileStream)
{

    fileStream.clear();

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open File", g_mainFrame->m_LastDirName,"File (*.*)");

    QFile File(fileName);
    if (!File.open(QIODevice::ReadOnly))
        return false;

    QTextStream in(&File);

    UpdateLastDirName(fileName);

    int pos = fileName.lastIndexOf("/");
    pos = fileName.size()-pos-1;
    fileName = fileName.right(pos);


    while(!in.atEnd())
    {
        QString strong;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);

        bool valid = true;
        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++) if (!ANY_NUMBER.match(list.at(i)).hasMatch()) valid = false;

        if (valid) fileStream.append(strong);
    }

    File.close();

    QList< QList <double> > data;

    for (int i=0;i<fileStream.size();i++){

        QList<double> datarow;

        bool valid = true;

        QStringList list = fileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
             valid = false;
             }
        }

        if (!valid) fileStream.removeAt(i);


        if (valid && list.size() > 1){
                for (int j=0;j<list.size();j++) datarow.append(list.at(j).toDouble());
                data.append(datarow);
        }
    }

    bool valid = true;
    if (data.size()<2) valid = false;

    for (int i=0;i<data.size()-1;i++){
        if (data.at(i).size() != data.at(0).size()) valid = false;
        if (data.at(i+1).at(0) <= data.at(i).at(0)) valid = false;
    }

    if (!valid){
        QString strange = "Could not interpret the file\n"+fileName;
        if (isGUI) QMessageBox::warning(g_mainFrame, "Warning", strange);
        else qDebug() << "Warning: Could not interpret the file\n"+fileName;
        return false;
    }

    return true;
}

void onExportFoil(QString FileName, Airfoil *foil){

    if (!foil)
        foil = g_pCurFoil;

    if(!foil)
        return;

    if (!FileName.size()){

        FileName = foil->getName();
        FileName.replace("/", " ");

        FileName = QFileDialog::getSaveFileName(g_mainFrame, "Export Foil",
                                                g_mainFrame->m_LastDirName+"/"+FileName,
                                                "Foil File (*.afl)");

    }

    if(!FileName.length()) return;
    int pos = FileName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = FileName.left(pos);

    QFile XFile(FileName);

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;

    QTextStream out(&XFile);

    foil->ExportFoil(out);
    XFile.close();

}

void exportFoil(QString FileName, Airfoil *foil){

    if (!foil)
        foil = g_pCurFoil;

    if(!foil)
        return;

    if (!FileName.size())
        return;

    if(!FileName.length()) return;
    int pos = FileName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = FileName.left(pos);

    QFile XFile(FileName);

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;

    QTextStream out(&XFile);

    foil->ExportFoil(out);
    XFile.close();

}


Airfoil *interpolateFoils(Airfoil *foil1, Airfoil *foil2, double frac){

    if (!foil1 || !foil2) return NULL;

    Airfoil *foil = NULL;

    QString foil1ID = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString foil2ID = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString xfoilID = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString tmpID = QUuid::createUuid().toString(QUuid::WithoutBraces);


    if (!QDir(g_tempPath).exists())
        QDir().mkdir(g_tempPath);

    QString directory = QString(g_tempPath)+QDir::separator();

    exportFoil(directory+foil1ID+".dat",foil1);
    exportFoil(directory+foil2ID+".dat",foil2);

    QFile XFile(directory+xfoilID+".txt");

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "Error, cant create xfbat.txt!!";
        return NULL;
    }

    QTextStream out(&XFile);

    out << "PLOP\n";
    out << "G F\n";
    out << "\n";
    out << "LOAD "+directory+foil1ID+".dat\n";
    out << "INTE\n";
    out << "C\n";
    out << "F\n";
    out << directory+foil2ID+".dat\n";
    out << QString().number(frac,'f',2)+"\n";
    out << "Interpolated\n";
    out << "PCOP\n";
    out << "PPAR\n";
    out << "N"+QString().number(foil1->n*0.5+foil2->n*0.5,'f',0)+"\n";
    out << "\n";
    out << "\n";
    out << "PANE\n";
    out << "SAVE "+directory+tmpID+".dat\n";
    out << "\n";
    out << "QUIT\n";

    XFile.close();

    BinaryProgressDialog *progressDialog = new BinaryProgressDialog ("XFoil",false);
    progressDialog->setStandardInputFile(directory+xfoilID+".txt");
    progressDialog->startProcess(QStringList());

    progressDialog->getProcess()->waitForFinished(1000);

    if (XFile.exists())
        XFile.remove();

    QFile FFileA(directory+foil1ID+".dat");
    if (FFileA.exists())
        FFileA.remove();


    QFile FFileB(directory+foil2ID+".dat");
    if (FFileB.exists())
        FFileB.remove();

    QFile FFile(directory+tmpID+".dat");
    if (FFile.exists()){
        foil = ImportAirfoil(directory+tmpID+".dat");
        FFile.remove();
    }

    return foil;

}

Airfoil *generateNACAFoil(int digits, int panels){

    Airfoil *foil = NULL;

    if (!QDir(g_tempPath).exists())
        QDir().mkdir(g_tempPath);

    QString directory = QString(g_tempPath)+QDir::separator();

    if (panels%2 == 0) panels+=1;

    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QFile XFile(directory+uuid+".txt");

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "Error, cant create xfbat.txt!!";
        return NULL;
    }

    QTextStream out(&XFile);

    out << "PLOP\n";
    out << "G F\n";
    out << "\n";
    out << "NACA "+QString().number(digits,'f',0)+"\n";
    out << "PPAR\n";
    out << "N"+QString().number(panels,'f',0)+"\n";
    out << "\n";
    out << "\n";
    out << "PANE\n";
    out << "SAVE "+directory+uuid+".dat\n";
    out << "\n";
    out << "QUIT\n";

    XFile.close();

    BinaryProgressDialog *progressDialog = new BinaryProgressDialog ("XFoil",false);
    progressDialog->setStandardInputFile(directory+uuid+".txt");
    progressDialog->startProcess(QStringList());

    progressDialog->getProcess()->waitForFinished(1000);

    if (XFile.exists()){
        XFile.remove();
    }

    QFile FFile(directory+uuid+".dat");

    if (FFile.exists()){
        foil = ImportAirfoil(directory+uuid+".dat");
        FFile.remove();
    }

    return foil;
}
void printStoreState () {

    qDebug() << "Store: Printing Store States\n";
    g_windFieldStore.printState();
    g_foilStore.printState();
    g_polarStore.printState();
    g_360PolarStore.printState();
    g_DynPolarSetStore.printState();
    g_rotorStore.printState();
    g_verticalRotorStore.printState();
    g_bladeStructureStore.printState();
    g_tdataStore.printState();
    g_verttdataStore.printState();
    g_bemdataStore.printState();
    g_tbemdataStore.printState();
    g_cbemdataStore.printState();
    g_propbemdataStore.printState();
    g_propcbemdataStore.printState();
    g_dmsdataStore.printState();
    g_tdmsdataStore.printState();
    g_cdmsdataStore.printState();
    g_bladestructureloadingStore.printState();
    g_noiseSimulationStore.printState();
    g_StrutStore.printState();
    g_FlapStore.printState();
    g_BDamageStore.printState();
    g_QTurbinePrototypeStore.printState();
    g_QSimulationStore.printState();
    g_QVelocityCutPlaneStore.printState();
    g_QTurbineSimulationStore.printState();
    g_StrModelMultiStore.printState();
    g_WaveStore.printState();
    g_operationalPointStore.printState();

}

QStringList FindStreamSegmentByKeyword(QString keyword, QStringList File){

    QStringList segment;

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    bool readIn = false;

    for (int i=0; i<FileContents.size();i++){

        for (int j=0; j<FileContents.at(i).size();j++){
            if ("END_"+keyword == FileContents.at(i).at(j)) readIn = false;
        }

        if (readIn) segment.append(File.at(i));

        for (int j=0; j<FileContents.at(i).size();j++){
            if (keyword == FileContents.at(i).at(j)) readIn = true;
        }

    }

    return segment;

}


void ConstrainAngle_0_360_Degree(double &ang){
    ang = fmod(ang,360.0);
    if (ang < 0) ang += 360.0;
}

void ConstrainAngle_0_360_Degree(float &ang){
    ang = fmod(ang,360.0);
    if (ang < 0) ang += 360.0;
}

void ConstrainAngle_0_360_Radian(double &ang){
    ang = fmod(ang,2.0*PI_);
    if (ang < 0) ang += 2.0*PI_;
}

void ConstrainAngle_0_360_Radian(float &ang){
    ang = fmod(ang,2.0*PI_);
    if (ang < 0) ang += 2.0*PI_;
}

void ConstrainAngle_180_180_Degree(double &ang){
    ang = fmod(ang+180.0,360.0);
    if (ang < 0) ang += 360.0;
    ang -= 180.0;
}

void ConstrainAngle_180_180_Degree(float &ang){
    ang = fmod(ang+180.0,360.0);
    if (ang < 0) ang += 360.0;
    ang -= 180.0;
}

void ConstrainAngle_180_180_Radian(double &ang){
    ang = fmod(ang+PI_,2.0*PI_);
    if (ang < 0) ang += 2.0*PI_;
    ang -= PI_;
}

void ConstrainAngle_180_180_Radian(float &ang){
    ang = fmod(ang+PI_,2.0*PI_);
    if (ang < 0) ang += 2.0*PI_;
    ang -= PI_;
}

void CalculatePSD(QVector<float> *data, QVector<float> &xResult, QVector<float> &yResult, double dT){

    int size = data->size();

    if (size > 200){

        int resSize = size/2+1;

        float *RE = new float[resSize];
        float *IM = new float[resSize];


        // initialize REX[] and IMX[] so they can be used as accumulators
        for (int k = 0; k < resSize; k++)
        {
            RE[k] = 0;
            IM[k] = 0;
        }

        // loop through each sample in the frequency domain
        #pragma omp parallel default (none) shared (data,RE,IM,size,resSize)
        {
        #pragma omp for
            for (int k = 0; k < resSize; k++){
                // loop through each sample in the time domain
                for (int l = 0; l < size; l++){
                    float arg = float(2.0f*PI_*k*l/size);
                    RE[k] += data->at(l) * cos(arg);
                    IM[k] += -data->at(l) * sin(arg);
                }
            }
        }

        double dF = 1./dT;

        for (int k=1;k<resSize;k++){
            yResult.append(RE[k]*RE[k]+IM[k]*IM[k]*2.0*2.0/size/size);
            xResult.append(float(k)*dF/(size));
        }

        delete [] RE;
        delete [] IM;
    }
}

void CalculatePSD2(QVector<float> *data, QVector<float> &freq, QVector<float> &amp, QVector<float> &phase, double dT){

    int size = data->size();

    if (size > 200){

        int resSize = size/2+1;

        float *RE = new float[resSize];
        float *IM = new float[resSize];


        // initialize REX[] and IMX[] so they can be used as accumulators
        for (int k = 0; k < resSize; k++)
        {
            RE[k] = 0;
            IM[k] = 0;
        }

        // loop through each sample in the frequency domain
        #pragma omp parallel default (none) shared (data,RE,IM,size,resSize)
        {
        #pragma omp for
            for (int k = 0; k < resSize; k++){
                // loop through each sample in the time domain
                for (int l = 0; l < size; l++){
                    float arg = float(2.0f*PI_*k*l/size);
                    RE[k] += data->at(l) * cos(arg);
                    IM[k] += -data->at(l) * sin(arg);
                }
            }
        }

        double dF = 1./dT;

        for (int k=1;k<resSize;k++){
            amp.append(sqrt(RE[k]*RE[k]+IM[k]*IM[k])/size*2);
            phase.append(atan2(-IM[k],RE[k]));
            freq.append(float(k)*dF/(size));
        }

        delete [] RE;
        delete [] IM;
    }
}
