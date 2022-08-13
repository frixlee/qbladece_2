/**********************************************************************

    Copyright (C) 2020 Joseph Saverin <joseph.saverin@qblade.org>

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

//-------------------------------------------------
// Simple implementation of debugging members------
//-------------------------------------------------

#ifndef DEBUGGING_H
#define DEBUGGING_H

#include    "Math_Types.h"  // Math types
#include    <QDebug>        // Debugging functions
#include    <QTime>         // Time features
#include    <QElapsedTimer> // Timing capabilities
#include    <QFile>         // Logfile
#include    <QThread>       // Thread pause

// Memory leak checking:

//#include <vld.h>

//--------Static debugging members----------

class Timer
{
    // This is a simple timer class meant to store and update data for a given function.
    QElapsedTimer T_int;
    QElapsedTimer T_tot;
    std::vector<qint64> Times;
    std::vector<char*> Names;

public:

    // Constructor
    Timer()     {T_int.start(); T_tot.start();}

    // Functions
    void    Add_Entry(char *Label)
    {
        Times.push_back(T_int.restart());
        Names.push_back(Label);
    }

    void Print_Time_Single()
    {
        qint64 Total_T = T_tot.restart();
        qDebug() << "Time here was" << Total_T << " ms";
    }

    // Destructor
    void Print_Time()
    {
        QString S;
        for (int i=0; i<Times.size(); i++)
        {
            S.append(Names[i]);
            S.append(" = ");
            S.append(QString::number(Times[i]));
            if (i!=Times.size()-1) S.append(", ");
        }
        qDebug() << S;
    }

    void Print_Time_Relative()
    {
        qint64 Total_T = T_tot.restart();
        int N = Times.size();
        QString S;
        for (int i=0; i<Times.size(); i++)
        {
            S.append(Names[i]);
            S.append(" = ");
            double N = double(Times[i]);
            double D = double(Total_T);
            S.append(QString::number(N/D*100,'f',1));
            S.append("%");
            if (i!=N-1) S.append(", ");
        }
        qDebug() << S;
    }

    void Print_Time_Naked(int NP, Real Grid, int P)
    {
        qint64 Total_T = T_tot.restart();
        QString S;
        S.append("Time Usage: ");
        S.append(QString::number(NP));       S.append(" ");
//        S.append(QString::number(Grid));    S.append(" ");
        S.append(QString::number(P));       S.append(" ");
        S.append(QString::number(Total_T,'f',1));   S.append(" ");
        for (int i=0; i<Times.size(); i++)
        {
            double N = double(Times[i]);
            double D = double(Total_T);
            S.append(QString::number(N/D*100,'f',1));
            if (i!=N-1) S.append(" ");
        }
        qDebug() << S;
    }
};

inline void Debug_Matrix(const Matrix &M)
{
    // Debugs a matrix in a pretty way
    int C = M.cols();
    int R = M.rows();
    qDebug() << "";
    for (int r=0; r<R; r++)
    {
        QString Row;
//        for (int c=0; c<C; c++) Row += QString("%1").arg(int(M(r,c)), 12);
        for (int c=0; c<C; c++) Row += QString("%1").arg(M(r,c), 12);
        qDebug().noquote() << Row;
//        for (int c=0; c<C; c++) qDebug() << M(r,c);
    }
    qDebug() << "";
}

inline bool is_NaN(const Matrix &M)
{
    // Debugs a matrix in a pretty way
    int C = M.cols();
    int R = M.rows();
    for (int r=0; r<R; r++)
    {
        for (int c=0; c<C; c++)
        {
            if (std::isnan(M(r,c))) return true;
        }
    }
    return false;
}

inline void Pause()         {}

static bool abs_compare(const Real &a, const Real &b)   {return (std::abs(a) < std::abs(b));}

inline void Check_Max(Real &Max_Curr, Real Val)   {if (Val>Max_Curr) Max_Curr = Val;}

inline void Check_Min(Real &Min_Curr, Real Val)   {if (Val<Min_Curr) Min_Curr = Val;}

inline void Debug_Vector(const Vector &V, bool MPL = false)
{
    if (MPL)    qDebug() << "[" << V(0) << V(1) << "," << V(2) << "," << V(3) << "," << V(4) << "," << V(5) << "," << V(6) << "]";
    else        qDebug() << V(0) << V(1) << V(2) << V(3) << V(4) << V(5) << V(6) << V(7);
}

inline void Debug_Vector6(const Vector &V, bool MPL = false)
{
    if (MPL)    qDebug() << "[" << V(0) << V(1) << "," << V(2) << "," << V(3) << "," << V(4) << "," << V(5) << "," << V(6) << "]";
    else        qDebug() << V(0) << V(1) << V(2) << V(3) << V(4) << V(5);
}

inline void Debug_Particle(const Vector &V)
{
     qDebug() << V(0) << V(1) << V(2) << V(3) << V(4) << V(5) << V(6) << V(7);
}

inline void Debug_StateVector(const StateVector &V, bool MPL = false)   {for (int i=0; i<V.size(); i++) Debug_Vector6(V[i],MPL);}

inline void Debug_Positions(const std::string& VarName, const StateVector &Var)
{
    // Just debugs a set of positions with a given name
    qDebug() << QString::fromStdString(VarName);
    for (Vector V: Var) qDebug() << V(0) << V(1) << V(2);
}

inline void Check_Extremes(Real &Min_Curr, Real &Max_Curr, const Real &Val)
{
    if (Val>Max_Curr) Max_Curr = Val;
    if (Val<Min_Curr) Min_Curr = Val;
}

inline void Log(const std::string& prettyFunction)
{
    // Export file as text

    size_t colons = prettyFunction.find("::");
    size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
    size_t end = prettyFunction.rfind("(") - begin;
//        return prettyFunction.substr(begin,end) + "()";

    QString FilePath = "Output/Log.txt";
    QFile file(FilePath);

    if (file.open(QIODevice::ReadWrite | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << QString::fromStdString(prettyFunction.substr(begin,end)+"()") << endl;
    }
}

inline void Log(const std::string& prettyFunction, const std::string& X)
{
    // Export file as text

    size_t colons = prettyFunction.find("::");
    size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
    size_t end = prettyFunction.rfind("(") - begin;
//        return prettyFunction.substr(begin,end) + "()";

    QString FilePath = "Output/Log.txt";
    QFile file(FilePath);

    if (file.open(QIODevice::ReadWrite | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << QString::fromStdString(prettyFunction.substr(begin,end)+"()"+X) << endl;
    }
}

inline void Notimplemented(QString Class, QString A) {qDebug() << Class << ":: " << A << " is not yet implemented";}

inline void Clear_Log()
{
    // Export file as text

    QString FilePath = "Output/Log.txt";
    QFile file("Output/Log.txt");
    if (file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Append)); // Clear!
    file.close();
}

#define LOG_FUNC_ENTRY      Log(__PRETTY_FUNCTION__)
#define LOG_FUNC(x)         Log(__PRETTY_FUNCTION__,x)
#define CLEAR_LOG           Clear_Log()

#endif // DEBUGGING_H
