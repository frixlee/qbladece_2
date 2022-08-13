/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#include "OperationalPoint.h"

#include "src/Serializer.h"
#include "src/Store.h"
#include "src/Graph/NewCurve.h"
#include "src/PolarModule/Polar.h"
#include "src/FoilModule/Airfoil.h"

OperationalPoint::OperationalPoint(QString name, StorableObject *parent)
: StorableObject (name,parent), ShowAsGraphInterface (true)
{

    initializeOutputVectors();
}

NewCurve* OperationalPoint::newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType){

    if (graphType == NewGraph::OpPointGraph){
        if (!m_Data.size()) return NULL;

        const int xAxisIndex = m_availableVariables.indexOf(xAxis);
        const int yAxisIndex = m_availableVariables.indexOf(yAxis);

        if (xAxisIndex == -1 || yAxisIndex == -1) {
            return NULL;
        }
        else{

            NewCurve *curve = new NewCurve (this);
            curve->setAllPoints(m_Data[xAxisIndex]->data(),
                                m_Data[yAxisIndex]->data(),
                                n);  // numberOfRows is the same for all results
            return curve;
        }
    }

    return NULL;

}

void OperationalPoint::drawPressureCurves(QList<NewCurve *> &curveList, QString yAxis){

    if (!m_BLData.size()) return;

    const int yAxisIndex = m_availableBLVariables.indexOf(yAxis);
    if (yAxisIndex == -1) return;

    QVector<float> x;
    QVector<float> y;

    float cosa = cos(-m_alpha / 180.*PI_);
    float sina = sin(-m_alpha / 180.*PI_);


    for (int i=0;i<n;i++){

        float xval = m_X[i]+m_Xn[i]*fabs(m_BLData.at(yAxisIndex)->at(i)*0.05);
        float yval = m_Y[i]+m_Yn[i]*fabs(m_BLData.at(yAxisIndex)->at(i)*0.05);

        x.append((xval-0.5)*cosa - yval*sina + 0.5);
        y.append((xval-0.5)*sina + yval*cosa);
    }

    for (int i=0;i<n;i++){

        QVector<float> lineX;
        QVector<float> lineY;

        lineX.append((m_X[i]-0.5)*cosa - m_Y[i]*sina + 0.5);
        lineY.append((m_X[i]-0.5)*sina + m_Y[i]*cosa);

        lineX.append(x[i]);
        lineY.append(y[i]);

        NewCurve *curve = new NewCurve ();
        curve->getAssociatedObject()->pen()->setColor(QColor(0,0,255));

        if (m_BLData.at(yAxisIndex)->at(i) > 0)
                curve->getAssociatedObject()->pen()->setColor(QColor(255,0,0));


        curve->setCurveName(getName());
        curve->setAllPoints(lineX.data(),lineY.data(),lineX.size());
        curveList.append(curve);

    }

    //the c/4 point
    {

        QVector<float> lineX;
        QVector<float> lineY;

        lineX.append((0.25-0.5)*cosa - 0*sina + 0.5);
        lineY.append((0.25-0.5)*sina + 0*cosa);

        NewCurve *curve = new NewCurve ();

        curve->getAssociatedObject()->pen()->setColor(QColor(0,0,0));
        curve->setCurveName(getName());
        curve->getAssociatedObject()->setDrawPoints(true);
        curve->getAssociatedObject()->pen()->setWidth(1);
        curve->setAllPoints(lineX.data(),lineY.data(),lineX.size());
        curveList.append(curve);

    }

    double arm = m_CM/(-m_CL*cos(m_alpha / 180.*PI_)-m_CD*sin(m_alpha / 180.*PI_))+0.25;
    //the force acting point
    {

        QVector<float> lineX;
        QVector<float> lineY;

        lineX.append((arm-0.5)*cosa - 0*sina + 0.5);
        lineY.append((arm-0.5)*sina + 0*cosa);

        NewCurve *curve = new NewCurve ();

        curve->getAssociatedObject()->pen()->setColor(QColor(255,0,0));
        curve->setCurveName(getName());
        curve->getAssociatedObject()->setDrawPoints(true);
        curve->getAssociatedObject()->pen()->setWidth(1);
        curve->setAllPoints(lineX.data(),lineY.data(),lineX.size());
        curveList.append(curve);

    }

    //CL
    {
        QVector<float> lineX;
        QVector<float> lineY;

        lineX.append((arm-0.5)*cosa + 0.5);
        lineY.append((arm-0.5)*sina);

        lineX.append(lineX.at(0));
        lineY.append(lineY.at(0)+m_CL*0.1);

        NewCurve *curve = new NewCurve ();

        curve->getAssociatedObject()->pen()->setColor(QColor(255,0,0));
        curve->setCurveName(getName());
        curve->getAssociatedObject()->pen()->setWidth(2);
        curve->setAllPoints(lineX.data(),lineY.data(),lineX.size());
        curveList.append(curve);
    }

    //CD
    {
        QVector<float> lineX;
        QVector<float> lineY;

        lineX.append((arm-0.5)*cosa + 0.5);
        lineY.append((arm-0.5)*sina);

        lineX.append(lineX.at(0)+m_CD*0.1);
        lineY.append(lineY.at(0));

        NewCurve *curve = new NewCurve ();

        curve->getAssociatedObject()->pen()->setColor(QColor(0,0,255));
        curve->setCurveName(getName());
        curve->getAssociatedObject()->pen()->setWidth(2);
        curve->setAllPoints(lineX.data(),lineY.data(),lineX.size());
        curveList.append(curve);
    }





    return;

}

void OperationalPoint::drawBLCurves(QList<NewCurve *> &curveList, QString yAxis){

    if (!m_BLData.size()) return;
    const int yAxisIndex = m_availableBLVariables.indexOf(yAxis);
    if (yAxisIndex == -1) return;

    QVector<float> x;
    QVector<float> y;

    float cosa = cos(-m_alpha / 180.*PI_);
    float sina = sin(-m_alpha / 180.*PI_);

    //Boundary Layer

    double tgap = m_Y[0];
    double bgap = m_Y[n-1];
    double factt = (m_DStar[0]*m_Yn[0]+tgap)/m_DStar[n];
    double factb = (m_DStar[n-1]*m_Yn[n-1]+bgap)/m_DStar[n];

    for (int i=0;i<n;i++){

        float xval = m_X[i]+m_Xn[i]*fabs(m_BLData.at(yAxisIndex)->at(i));
        float yval = m_Y[i]+m_Yn[i]*fabs(m_BLData.at(yAxisIndex)->at(i));

        x.append((xval-0.5)*cosa - yval*sina + 0.5);
        y.append((xval-0.5)*sina + yval*cosa);
    }

    for (int i=n;i<m_X.size();i++){

        float xvalt = m_X[i]-m_Xn[i]*fabs(m_BLData.at(yAxisIndex)->at(i))*factt;
        float yvalt = m_Y[i]-m_Yn[i]*fabs(m_BLData.at(yAxisIndex)->at(i))*factt;

        float xvalb = m_X[i]-m_Xn[i]*fabs(m_BLData.at(yAxisIndex)->at(i))*factb;
        float yvalb = m_Y[i]-m_Yn[i]*fabs(m_BLData.at(yAxisIndex)->at(i))*factb;

        x.insert(0,(xvalt-0.5)*cosa - yvalt*sina + 0.5);
        y.insert(0,(xvalt-0.5)*sina + yvalt*cosa);

        x.append((xvalb-0.5)*cosa - yvalb*sina + 0.5);
        y.append((xvalb-0.5)*sina + yvalb*cosa);
    }

    NewCurve *curve = new NewCurve ();
    curve->getAssociatedObject()->pen()->setColor(QColor(255,0,0));
    curve->setCurveName(getName());
    curve->getAssociatedObject()->pen()->setStyle(Qt::DashLine);

    curve->setAllPoints(x.data(),
                        y.data(),
                        x.size());  // numberOfRows is the same for all results

    curveList.append(curve);

    return;

}

void OperationalPoint::drawVelocityCurves(QList<NewCurve *> &curveList, QString yAxis){

    if (!m_BLData.size()) return;

    const int yAxisIndex = m_availableBLVariables.indexOf(yAxis);
    if (yAxisIndex == -1) return;

    QVector<float> x;
    QVector<float> y;

    float cosa = cos(-m_alpha / 180.*PI_);
    float sina = sin(-m_alpha / 180.*PI_);


    for (int i=0;i<n;i++){

        float xval = m_X[i]+m_Xn[i]*fabs(m_BLData.at(yAxisIndex)->at(i)*0.05);
        float yval = m_Y[i]+m_Yn[i]*fabs(m_BLData.at(yAxisIndex)->at(i)*0.05);

        x.append((xval-0.5)*cosa - yval*sina + 0.5);
        y.append((xval-0.5)*sina + yval*cosa);
    }

    for (int i=0;i<n;i++){

        QVector<float> lineX;
        QVector<float> lineY;

        lineX.append((m_X[i]-0.5)*cosa - m_Y[i]*sina + 0.5);
        lineY.append((m_X[i]-0.5)*sina + m_Y[i]*cosa);

        lineX.append(x[i]);
        lineY.append(y[i]);

        NewCurve *curve = new NewCurve ();
        curve->getAssociatedObject()->pen()->setColor(QColor(0,0,255));

        if (m_BLData.at(yAxisIndex)->at(i) > 0)
                curve->getAssociatedObject()->pen()->setColor(QColor(255,0,0));


        curve->setCurveName(getName());
        curve->setAllPoints(lineX.data(),lineY.data(),lineX.size());
        curveList.append(curve);

    }

    return;

}

void OperationalPoint::drawFoilCurve(QList<NewCurve *> &curveList){

    if (!m_BLData.size()) return;

    float cosa = cos(-m_alpha / 180.*PI_);
    float sina = sin(-m_alpha / 180.*PI_);

    QVector<float> x;
    QVector<float> y;

    for (int i=0;i<m_X.size();i++){
        x.append((m_X[i]-0.5)*cosa - m_Y[i]*sina + 0.5);
        y.append((m_X[i]-0.5)*sina + m_Y[i]*cosa);
    }

    Airfoil *foil = (Airfoil *) getParent()->getParent();
    NewCurve *curve = new NewCurve ();
    curve->getAssociatedObject()->pen()->setColor(QColor(130,130,130));
    curve->setCurveName(foil->getName()+"_Curve");

    curve->setAllPoints(x.data(),
                        y.data(),
                        n);  // numberOfRows is the same for all results



    curveList.append(curve);

    return ;

}

void OperationalPoint::CalculatePointNormals(){

    m_Xn.resize(m_X.size());
    m_Yn.resize(m_X.size());

    for (int i=1;i<m_X.size()-1;i++){

        Vec3 fVec = Vec3(m_X[i+1]-m_X[i],m_Y[i+1]-m_Y[i],0);
        Vec3 bVec = Vec3(m_X[i]-m_X[i-1],m_Y[i]-m_Y[i-1],0);

        fVec.Normalize();
        bVec.Normalize();

        double X = ( fVec.y + bVec.y)/2.0;
        double Y = (-fVec.x - bVec.x)/2.0;

        m_Xn[i] = X;
        m_Yn[i] = Y;

        if (i == 1){
            m_Xn[0] = bVec.y;
            m_Yn[0] = -bVec.x;
        }

        if (i == m_X.size()-2){
            m_Xn[i+1] = fVec.y;
            m_Yn[i+1] = -fVec.x;
        }

        // foild to bl transition
        if (i == n-1){
            m_Xn[i] = bVec.y;
            m_Yn[i] = -bVec.x;
        }

        if (i == n){
            m_Xn[i] = fVec.y;
            m_Yn[i] = -fVec.x;
        }

    }

}

void OperationalPoint::Copy(OperationalPoint *opPoint){

    m_X = opPoint->m_X;
    m_Y = opPoint->m_Y;
    m_UeVinf = opPoint->m_UeVinf;
    m_DStar = opPoint->m_DStar;
    m_Theta = opPoint->m_Theta;
    m_Cf = opPoint->m_Cf;
    m_H = opPoint->m_H;
    m_Cp = opPoint->m_Cp;
    n = opPoint->n;
    numTop = opPoint->numTop;
    numBot = opPoint->numBot;
    m_reynolds = opPoint->m_reynolds;
    m_mach = opPoint->m_mach;
    m_alpha = opPoint->m_alpha;

    initializeOutputVectors();
    sortArrays();

}

void OperationalPoint::initializeOutputVectors (){

    m_availableVariables.clear();
    m_Data.clear();

    m_availableVariables.append("X Position [-]");
    m_Data.append(&m_X);
    m_availableVariables.append("Y Position [-]");
    m_Data.append(&m_Y);
    m_availableVariables.append("Edge Velocity UeVinf [-]");
    m_Data.append(&m_UeVinf);
    m_availableVariables.append("Boundary Layer Thickness DStar [-]");
    m_Data.append(&m_DStar);
    m_availableVariables.append("Momentum Thickness Theta [-]");
    m_Data.append(&m_Theta);
    m_availableVariables.append("Skin Friction Coefficient Cf [-]");
    m_Data.append(&m_Cf);
    m_availableVariables.append("Kinematic Shape Parameter H [-]");
    m_Data.append(&m_H);
    m_availableVariables.append("Pressure Coefficient Cp (*-1) [-]");
    m_Data.append(&m_Cp);

    m_availableBLVariables.clear();
    m_BLData.clear();

    m_availableBLVariables.append("Pressure Distribution");
    m_BLData.append(&m_Cp);
    m_availableBLVariables.append("Velocity Distribution");
    m_BLData.append(&m_UeVinf);
    m_availableBLVariables.append("Boundary Layer");
    m_BLData.append(&m_DStar);

}

OperationalPoint* OperationalPoint::newBySerialize() {

    OperationalPoint* opPoint = new OperationalPoint ();
    opPoint->serialize();
    return opPoint;
}

void OperationalPoint::sortArrays(){

    for (int k=0;k<=numTop;k++){

        int index = numTop - k;

        m_X_Top.append(m_X.at(index));
        m_Y_Top.append(m_Y.at(index));
        m_UeVinf_Top.append(m_UeVinf.at(index));
        m_DStar_Top.append(m_DStar.at(index));
        m_Theta_Top.append(m_Theta.at(index));
        m_Cf_Top.append(m_Cf.at(index));
        m_H_Top.append(m_H.at(index));
        m_Cp_Top.append(m_Cp.at(index));
    }

    for (int k=numTop;k<=numTop+numBot;k++){

        int index = k;

        m_X_Bot.append(m_X.at(index));
        m_Y_Bot.append(m_Y.at(index));
        m_UeVinf_Bot.append(m_UeVinf.at(index));
        m_DStar_Bot.append(m_DStar.at(index));
        m_Theta_Bot.append(m_Theta.at(index));
        m_Cf_Bot.append(m_Cf.at(index));
        m_H_Bot.append(m_H.at(index));
        m_Cp_Bot.append(m_Cp.at(index));
    }

    for (int k=numTop+numBot;k<m_X.size();k++){

        int index = k;

        m_X_Wake.append(m_X.at(index));
        m_Y_Wake.append(m_Y.at(index));
        m_UeVinf_Wake.append(m_UeVinf.at(index));
        m_DStar_Wake.append(m_DStar.at(index));
        m_Theta_Wake.append(m_Theta.at(index));
        m_Cf_Wake.append(m_Cf.at(index));
        m_H_Wake.append(m_H.at(index));
    }

    CalculatePointNormals();

//            qDebug() << "TOP";
//            for (int t=0;t<m_X_Top.size();t++)
//                qDebug() << m_X_Top[t] << m_Y_Top[t];
//            qDebug() << "BOT";
//            for (int t=0;t<m_X_Bot.size();t++)
//                qDebug() << m_X_Bot[t] << m_Y_Bot[t];
//            qDebug() << "WAKE";
//            for (int t=0;t<m_X_Wake.size();t++)
//                qDebug() << m_X_Wake[t] << m_Y_Wake[t];

}

void OperationalPoint::serialize() {
    StorableObject::serialize();
    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteInt(&n);
    g_serializer.readOrWriteInt(&numTop);
    g_serializer.readOrWriteInt(&numBot);

    g_serializer.readOrWriteDouble(&m_reynolds);
    g_serializer.readOrWriteDouble(&m_mach);
    g_serializer.readOrWriteDouble(&m_alpha);

    g_serializer.readOrWriteDouble(&m_CL);
    g_serializer.readOrWriteDouble(&m_CD);
    g_serializer.readOrWriteDouble(&m_CM);

    g_serializer.readOrWriteFloatVector1D(&m_X);
    g_serializer.readOrWriteFloatVector1D(&m_Y);
    g_serializer.readOrWriteFloatVector1D(&m_UeVinf);
    g_serializer.readOrWriteFloatVector1D(&m_DStar);
    g_serializer.readOrWriteFloatVector1D(&m_Theta);
    g_serializer.readOrWriteFloatVector1D(&m_Cf);
    g_serializer.readOrWriteFloatVector1D(&m_H);
    g_serializer.readOrWriteFloatVector1D(&m_Cp);

    sortArrays();

}

