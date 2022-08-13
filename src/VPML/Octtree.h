/**********************************************************************

    Copyright (C) 2019 Joseph Saverin <joseph.saverin@qblade.org>

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

#include "Math_Types.h"
#include "Debug.h"

#ifndef OCTTREE_H
#define OCTTREE_H

typedef unsigned int    uint;       // Shorthand

// Tree ID type.
static uint const Tree_Lim = 12;     // Root node level

// Tree limits are dictated by +/- H_Grid*2**Tree_Lim;

typedef std::vector<uint> TreeID;   // Tree ID Type

static TreeID TreeTemp = [] {TreeID V; V.assign(Tree_Lim+1,0); return V;}();

//---------------------------------
//-------Base Node class-----------
//---------------------------------

template <typename T>
class Tree_Node
{
protected:

    std::shared_ptr<T> Obj = nullptr;       // The object at this leaf
    std::vector<Tree_Node*> Sub_Nodes;      // The sub branches of this node (4-> 2D tree, 8-> 3D tree)
//    SafeVector<Tree_Node*> Sub_Nodes;      // The sub branches of this node (4-> 2D tree, 8-> 3D tree)


public:

    Tree_Node()     {}  // Empty constructor

    // Access functions

    std::shared_ptr<T>  Get_Object() {return Obj;}
    void                Set_Object(std::shared_ptr<T> O)
    {
//        Mutex.lock();
        Obj = O;
//        Mutex.unlock();
    }

    // Access children

    void Get_Sub_Nodes(std::vector<Tree_Node<T>*> &L)  {StdAppend(L,Sub_Nodes);}

    //--- Traversal functions

    // Traverse tree for leaf node
    Tree_Node* Get_Leaf(const TreeID &TID, const uint &Level=Tree_Lim, const uint &ind=0)
    {
        if (ind==Level+1) return this;                          // We have reached desired level
        if (Sub_Nodes.empty())  Populate_Sub_Nodes();           // We need to subdivide
        return Sub_Nodes[TID[ind]]->Get_Leaf(TID,Level,ind+1);  // return Quadrant/octant of interest
    }

    //---  Cartesian-quadrant description

    virtual void Get_TID(const Cart_ID &ID,                         // Cartesian ID
                         TreeID &TID,                               // Tree ID
                         const uint &Level = 0,                     // Desired level in tree
                         const uint &Br = 0,                        // Initial branch is zero = Root node
                         const Cart_ID &Centre = Cart_ID(0,0,0)){}  // Initial Centre is zero

    virtual Cart_ID Get_CID(const Vector3 &Pos, const Real &H, const uint &Level=0) {}

    virtual Vector3 Get_Centre( const TreeID &TID,                          // Tree ID
                                const Real &H,
                                const uint &Level = 0,                      // Desired level in tree
                                const uint &Br = 0,                         // Initial branch is zero = Root node
                                const Vector3 &Centre = Vector3(0,0,0)){}   // Initial Centre is zero

    virtual Matrix Get_Corners(const TreeID &TID, const Real &H, const uint &Level = 0) {}  // Return box corners

    //--- Populate tree

    virtual void Populate_Sub_Nodes()  {} // Include flag to create leafs

    ~Tree_Node()
    {
        for (Tree_Node* N : Sub_Nodes) delete N;
        Obj.reset();
    }
};

//---------------------------------
//-------Quad_Tree-----------------
//---------------------------------

template <typename T>
class Quad_Tree : public Tree_Node<T>
{
    // Implements the functionality for 2D octtrees

    using Tree_Node<T>::Sub_Nodes;
    using Tree_Node<T>::Obj;

public:

    Quad_Tree() : Tree_Node<T>()    {}

    //---  Cartesian-quadrant description

    void Get_TID(const Cart_ID &ID,                         // Cartesian ID
                      TreeID &TID,                          // Tree ID
                      const uint &Level = 0,                // Desired level in tree
                      const uint &Br = 0,                   // Initial branch is zero = Root node
                      const Cart_ID &Centre = Cart_ID(0,0,0))// Initial Centre is zero
    {
        // Recursive function to specify TID
        // Note: ID is specified at ROOT level!

        int Quad=0;
        int BS = 1<<(Tree_Lim-Br-1);    // Boxlength side
        Cart_ID NewCentre = Centre;

        // X quadrant
        if (ID(0)-Centre(0) >= 0)
        {
                Quad += 2;
                NewCentre(0) += BS;
        }
        else    NewCentre(0) -= BS;

        // Y quadrant
        if (ID(1)-Centre(1) >= 0)
        {
                Quad += 1;
                NewCentre(1) += BS;
        }
        else    NewCentre(1) -= BS;

        TID[Br] = Quad;
        if (Br == (Tree_Lim-Level))    return;
        return Get_TID(ID,TID,Level,Br+1,NewCentre);
    }

    Cart_ID Get_CID(const Vector3 &Pos, const Real &H, const uint &Level=0)
    {
        // This will return the TID by first identifying the cartesian ID of the location
        // Note: Pos must be in the local system

        int BS = 1<<Level;
        Real SideFac = 1.0/(H*BS);

        Real I = Pos(0)*SideFac;
        Real J = Pos(1)*SideFac;

        int II = int(I);
        int JJ = int(J);

        if (I<0) II--;
        if (J<0) JJ--;

        return Cart_ID(II,JJ,0);
    }

    //--- Populate tree

    void Populate_Sub_Nodes()
    {
        for (int i=0; i<4; i++) Sub_Nodes.push_back(new Quad_Tree());
//        for (int i=0; i<4; i++) Sub_Nodes.push_back_multi(new Quad_Tree());
    }

    //--- Geo functions

    Vector3 Get_Centre( const TreeID &TID,                      // Tree ID
                        const Real &H,
                        const uint &Level = 0,                  // Desired level in tree
                        const uint &Br = 0,                     // Initial branch is zero = Root node
                        const Vector3 &Centre = Vector3(0,0,0)) // Initial Centre is zero
    {

        // Calculates the centre of the box at the given position

        Real Exp = Real(int(Tree_Lim-Br))-1.0;
        Real SL = H*pow(2.0,Exp);
        Vector3 NewCentre = Centre;

        switch (TID[Br])
        {
            case 0:      NewCentre += Vector3(-SL,-SL,0);       break;
            case 1:      NewCentre += Vector3(-SL,SL,0);        break;
            case 2:      NewCentre += Vector3(SL,-SL,0);        break;
            case 3:      NewCentre += Vector3(SL,SL,0);         break;
            default: ;
        }

//        // X Pos
//        if (TID[Br]>1)      NewCentre(0) += SL;
//        else                NewCentre(0) -= SL;

//        // Y Pos
//        if (TID[Br]%2==1)   NewCentre(1) += SL;
//        else                NewCentre(1) -= SL;

        if (Br == (Tree_Lim-Level)) return NewCentre;
        return Get_Centre(TID,H,Level,Br+1,NewCentre);
    }

    Matrix Get_Corners(const TreeID &TID, const Real &H, const uint &Level = 0)   // Return box corners
    {
        // Returns the corner points

        Matrix Coorns = Matrix::Zero(4,3);
        Vector3 C = Get_Centre(TID, H, Level);
        Real H2 = 0.5*H*pow(2,Level);
        Vector3 SHFX; SHFX << 1, 0, 0;
        Vector3 SHFY; SHFY << 0, 1, 0;

        Coorns.row(0) = C - SHFX*H2 - SHFY*H2;
        Coorns.row(1) = C - SHFX*H2 + SHFY*H2;
        Coorns.row(2) = C + SHFX*H2 - SHFY*H2;
        Coorns.row(3) = C + SHFX*H2 + SHFY*H2;

        return Coorns;
    }
};

template <typename T>
class Oct_Tree : public Tree_Node<T>
{
    // Implements the functionality for 2D octtrees

    using Tree_Node<T>::Sub_Nodes;
    using Tree_Node<T>::Obj;

public:

    Oct_Tree() : Tree_Node<T>()    {}

    //---  Cartesian-quadrant description

    void Get_TID(const Cart_ID &ID,                         // Cartesian ID
                      TreeID &TID,                          // Tree ID
                      const uint &Level = 0,                // Desired level in tree
                      const uint &Br = 0,                   // Initial branch is zero = Root node
                      const Cart_ID &Centre = Cart_ID(0,0,0))// Initial Centre is zero
    {
        // Recursive function to specify TID
        // Note: ID is specified at ROOT level!

        int Quad=0;
        int BS = 1<<(Tree_Lim-Br-1);    // Boxlength side
        Cart_ID NewCentre = Centre;

        // X quadrant
        if (ID(0)-Centre(0) >= 0)
        {
                Quad += 4;
                NewCentre(0) += BS;
        }
        else    NewCentre(0) -= BS;

        // Y quadrant
        if (ID(1)-Centre(1) >= 0)
        {
                Quad += 2;
                NewCentre(1) += BS;
        }
        else    NewCentre(1) -= BS;

        if (ID(2)-Centre(2) >= 0)
        {
                Quad += 1;
                NewCentre(2) += BS;
        }
        else    NewCentre(2) -= BS;

        TID[Br] = Quad;
        if (Br == (Tree_Lim-Level))    return;
        return Get_TID(ID,TID,Level,Br+1,NewCentre);
    }

    Cart_ID Get_CID(const Vector3 &Pos, const Real &H, const uint &Level=0)
    {
        // This will return the TID by first identifying the cartesian ID of the location
        // Note: Pos must be in the local system

        int BS = 1<<Level;
        Real SideFac = 1.0/(H*BS);

        Real I = Pos(0)*SideFac;
        Real J = Pos(1)*SideFac;
        Real K = Pos(2)*SideFac;

        int II = int(I);
        int JJ = int(J);
        int KK = int(K);

        if (I<0) II--;
        if (J<0) JJ--;
        if (K<0) KK--;

        return Cart_ID(II,JJ,KK);
    }

    //--- Populate tree

    void Populate_Sub_Nodes()
    {
        for (int i=0; i<8; i++) Sub_Nodes.push_back(new Oct_Tree());
//        for (int i=0; i<4; i++) Sub_Nodes.push_back_multi(new QuadTree());
    }

    //--- Geo functions

    //--- Geo functions

    Vector3 Get_Centre( const TreeID &TID,                      // Tree ID
                        const Real &H,
                        const uint &Level = 0,                  // Desired level in tree
                        const uint &Br = 0,                     // Initial branch is zero = Root node
                        const Vector3 &Centre = Vector3(0,0,0)) // Initial Centre is zero
    {

        // Calculates the centre of the box at the given position

        Real Exp = Real(int(Tree_Lim-Br))-1.0;
        Real SL = H*pow(2.0,Exp);
        Vector3 NewCentre = Centre;

        switch (TID[Br])
        {
            case 0:      NewCentre += Vector3(-SL,-SL,-SL);     break;
            case 1:      NewCentre += Vector3(-SL,-SL,+SL);     break;
            case 2:      NewCentre += Vector3(-SL,+SL,-SL);     break;
            case 3:      NewCentre += Vector3(-SL,+SL,+SL);     break;
            case 4:      NewCentre += Vector3(SL,-SL,-SL);      break;
            case 5:      NewCentre += Vector3(SL,-SL,+SL);      break;
            case 6:      NewCentre += Vector3(SL,+SL,-SL);      break;
            case 7:      NewCentre += Vector3(SL,+SL,+SL);      break;
            default: ;
        }

        if (Br == (Tree_Lim-Level)) return NewCentre;
        return Get_Centre(TID,H,Level,Br+1,NewCentre);
    }

    Matrix Get_Corners(const TreeID &TID, const Real &H, const uint &Level = 0)   // Return box corners
    {
        // Returns the corner points

        Matrix Coorns = Matrix::Zero(8,3);
        Vector3 C = Get_Centre(TID, H, Level);
        Real H2 = 0.5*H*pow(2,Level);
        Vector3 SHFX; SHFX << 1, 0, 0;
        Vector3 SHFY; SHFY << 0, 1, 0;
        Vector3 SHFZ; SHFZ << 0, 0, 1;

        Coorns.row(0) = C - SHFX*H2 - SHFY*H2 - SHFZ*H2;
        Coorns.row(1) = C - SHFX*H2 - SHFY*H2 + SHFZ*H2;
        Coorns.row(2) = C - SHFX*H2 + SHFY*H2 - SHFZ*H2;
        Coorns.row(3) = C - SHFX*H2 + SHFY*H2 + SHFZ*H2;
        Coorns.row(4) = C + SHFX*H2 - SHFY*H2 - SHFZ*H2;
        Coorns.row(5) = C + SHFX*H2 - SHFY*H2 + SHFZ*H2;
        Coorns.row(6) = C + SHFX*H2 + SHFY*H2 - SHFZ*H2;
        Coorns.row(7) = C + SHFX*H2 + SHFY*H2 + SHFZ*H2;

        return Coorns;
    }
};

// Testing class
class Tree_Test
{

public:

    static void Test_Multilthreading()
    {
        QElapsedTimer T_m; T_m.start();

//        SafeVector<Real> SV;
//        int N_R = 10000000;
//        for (int i=0; i<N_R; i++)   SV.push_back(4.0);
//        qint64 td = T_m.restart();
//        SV.clear();
//        #pragma omp parallel for
//        for (int i=0; i<N_R; i++)
//        {
//            SV.push_back_multi(4.0);
//        }
//        qint64 tm = T_m.restart();

//        qDebug()    << "NPoints " << N_R  << ", td " << td  << ", tm " << tm;
    }

    static void Test_2D()
    {
        // Testing function 2D Tree test


        int N_Points = 1000000;

        Tree_Node<Vector> *Tree = new Quad_Tree<Vector>();
        // Create array of random positions which are multiples of 2

//        std::vector<Vector3> POS;
        std::vector<Cart_ID> CIDS;
        std::vector<TreeID> TIDS;
        int MaxVal = 1<<Tree_Lim;   // Bit shift

        //  Set IDs
        for (int i=0; i<N_Points; i++)
        {
            Real RandX = 2.0*MaxVal*rand()*1.0/RAND_MAX-1.0*MaxVal;
            Real RandY = 2.0*MaxVal*rand()*1.0/RAND_MAX-1.0*MaxVal;

            Vector3 Pos(RandX,RandY,0.0);
            CIDS.push_back(Cart_ID(int(RandX), int(RandY),0));
        }

        QElapsedTimer T_tot; T_tot.start();

//        SafeVector<TreeID> TIDS;

//        #pragma omp parallel for
        for (int i=0; i<N_Points; i++)
        {
            TreeID TID = TreeTemp;
            Tree->Get_TID(CIDS[i],TID);
            TIDS.push_back(TID);
//            TIDS.push_back_multi(TID);
        }

        qint64 timetid = T_tot.restart();

//        #pragma omp parallel for
        for (int i=0; i<N_Points; i++)      // Creates data race!
        {
            Tree_Node<Vector> *Leaf = Tree->Get_Leaf(TIDS[i]);
        }

        qint64 timeleaf = T_tot.restart();

        #pragma omp parallel for
        for (int i=0; i<N_Points; i++)
        {
            Tree_Node<Vector> *Leaf = Tree->Get_Leaf(TIDS[i]);
        }

        qint64 timeleaf2 = T_tot.restart();


//        for (int i=0; i<N_Points; i++)  Vector3 C0 = Tree->Get_Centre(TIDS[i],1.0);

//        qint64 timecentre = T_tot.restart();

        qDebug()    << "2D: MaxBr" << Tree_Lim  \
                    << ", NPoints " << N_Points      \
                    << ", timetid " << timetid      \
                    << ", timeleaf " << timeleaf    \
                    << ", timeleaf2 " << timeleaf2;//     \
//                    << ", timecentre " << timecentre;

//        delete Tree;
    }

    static void Test_3D()
    {
        // Testing function 2D Tree test


        int N_Points = 1000000;

        Tree_Node<Vector> *Tree = new Oct_Tree<Vector>();
        // Create array of random positions which are multiples of 2

//        std::vector<Vector3> POS;
        std::vector<Cart_ID> CIDS;
        std::vector<TreeID> TIDS;
        int MaxVal = 1<<Tree_Lim;   // Bit shift

        //  Set IDs
        for (int i=0; i<N_Points; i++)
        {
            Real RandX = 2.0*MaxVal*rand()*1.0/RAND_MAX-1.0*MaxVal;
            Real RandY = 2.0*MaxVal*rand()*1.0/RAND_MAX-1.0*MaxVal;
            Real RandZ = 2.0*MaxVal*rand()*1.0/RAND_MAX-1.0*MaxVal;

//            Vector3 Pos(RandX,RandY,RandZ);
            CIDS.push_back(Cart_ID(int(RandX), int(RandY),int(RandZ)));
        }

        QElapsedTimer T_tot; T_tot.start();

//        SafeVector<TreeID> TIDS;

//        #pragma omp parallel for
        for (int i=0; i<N_Points; i++)
        {
            TreeID TID = TreeTemp;
            Tree->Get_TID(CIDS[i],TID);
            TIDS.push_back(TID);
//            TIDS.push_back_multi(TID);
        }

        qint64 timetid = T_tot.restart();

//        #pragma omp parallel for
        for (int i=0; i<N_Points; i++)      // Creates data race!
        {
            Tree_Node<Vector> *Leaf = Tree->Get_Leaf(TIDS[i]);
        }

        qint64 timeleaf = T_tot.restart();

        #pragma omp parallel for
        for (int i=0; i<N_Points; i++)
        {
            Tree_Node<Vector> *Leaf = Tree->Get_Leaf(TIDS[i]);
        }

        qint64 timeleaf2 = T_tot.restart();


//        for (int i=0; i<N_Points; i++)  Vector3 C0 = Tree->Get_Centre(TIDS[i],1.0);

//        qint64 timecentre = T_tot.restart();

        qDebug()    << "3D: MaxBr" << Tree_Lim  \
                    << ", NPoints " << N_Points      \
                    << ", timetid " << timetid      \
                    << ", timeleaf " << timeleaf    \
                    << ", timeleaf2 " << timeleaf2;//     \
//                    << ", timecentre " << timecentre;

//        delete Tree;
    }
};

#endif // OCTTREE_H
