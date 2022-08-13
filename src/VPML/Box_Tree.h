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


#ifndef GRID_H
#define GRID_H

#include "Math_Types.h"
#include "Octtree.h"
#include "ML_Box.h"
#include "VPML_Gobal_Vars.h"

namespace VPML
{


inline bool X_GT_PerDom(const Vector &V)                    {return (V(0)>Vars->Lx_Per); }
inline Vector3 RelPos(const Vector &V)                      {return Vector3(V(0), V(1), V(2)) - Grid_Vars->Origin;}

class Box_Tree
{
protected:

    //--------------------------
    //--- Tree Variables
    //--------------------------

    Tree_Node<Box>     *Box_OctTree;        // Box tree
    int NF_crit = 1;            // Which region to we consider to be near field of a Receiver box?

    //--------------------------
    //--- Source-Probe Variables
    //--------------------------

    int N_Sources;
    int N_Probes;

    Real        H;
    SPBoxList   Boxes;
    SPBoxList   Active_Boxes;
    SPBoxList   ProActive_Boxes;

    //--- Periodic variables

//    int         NB_Periodic;
//    SPBoxList   ProActive_Periodic_Base;
    SPBoxList   ProActive_Boxes_Periodic;
//    void Periodic_Shift()
    virtual     void  Get_Repeated_Boxes(SPBoxList &L)          {}

    //--- Block grid vars

    SPBoxList Block_Grid;                               // The list of block grids
    Cart_ID  Block_Grid_Lower = Cart_ID(1e6,1e6,1e6);   // Lower grid index
    Cart_ID  Block_Grid_Upper =-Cart_ID(1e6,1e6,1e6);   // Upper grid index
    Vector3  Block_Corn = Vector3::Zero();
    Cart_ID  Block_Grid_Delta;                          // Grid size

    //--- Box search

    SPBox Get_Box(const Vector &V, const uint &Branch=0)
    {
        // Uses local position to find corresponding cartesian ID
        Vector3 Pos_Ref = Vector3(V(0), V(1), V(2)) - Grid_Vars->Origin;
        Cart_ID CID = Box_OctTree->Get_CID(Pos_Ref,H,Branch);
        return Get_Box(CID, Branch);
    }

    SPBox Get_Box(const Cart_ID &CID, const uint &Branch=0)
    {
        // Uses grid local cartesian ID to find Box
        TreeID TID = TreeTemp;
        Cart_ID CIDS = CID*(1<<Branch);  // Scale for higher branches
        Box_OctTree->Get_TID(CIDS,TID,Branch);

        Tree_Node<Box> *Leaf = Box_OctTree->Get_Leaf(TID,Tree_Lim-Branch);
        if (Leaf->Get_Object()==nullptr)  Create_Box(Leaf,TID,CID,Branch);
        return Leaf->Get_Object();
    }

    SPBox Get_Box(const Cart_ID &CID, const TreeID &TID, const uint &Branch=0)
    {
        // The TID is known, just extract box from corresponding position
        Tree_Node<Box> *Leaf = Box_OctTree->Get_Leaf(TID,Tree_Lim-Branch);
        if (Leaf->Get_Object()==nullptr)  Create_Box(Leaf,TID,CID,Branch);
        return Leaf->Get_Object();
    }

    SPBox Create_Box(   Tree_Node<Box> *Leaf,
                        const TreeID &TID,
                        const Cart_ID &CID,
                        const uint &Branch=0)
    {
        // Generic function for creating a box and assigning it to a leaf
        SPBox B = std::make_shared<Box>(TID,CID,Branch);
        Leaf->Set_Object(B);
        Boxes.push_back(B);
        return B;
    }

public:

    //--- Constructor

    Box_Tree()    {}              // Constructor gridsize

    //__ Grid init

    void Initialize_Grid()                       // Constructor gridsize
    {
        // Initialize boxtree
        H = Grid_Vars->H_ML;
        if (Vars->Dim==SIM_2D) Box_OctTree = new Quad_Tree<Box>();
        if (Vars->Dim==SIM_3D) Box_OctTree = new Oct_Tree<Box>();
//        qDebug() << "Particle Grid Initialized";
    }

    //----------------------------------------------
    //--------Source and Probe Binning--------------
    //----------------------------------------------

    void Bin_Sources(const StateVector &Src)
    {
        if (Vars->Grid_Option==TREE)     Bin_Sources_Tree(Src);
        if (Vars->Grid_Option==BLOCK)    Bin_Sources_Block(Src);
    }

//    //--- Nullifying source

    void Bin_Sources_Tree(const StateVector &Src);

    void Bin_Sources_Block(const StateVector &Src);

    void Clear_Grid();

    void Set_Block_Index(const Vector &V, Cart_ID4 &QInf);

    void Bin_Probes(const StateVector &Prb)
    {
        if (Vars->Prbs_Are_Srcs)    Set_Sources_to_Probes();
        else
        {
            if (Vars->Grid_Option==TREE)     Bin_Probes_Tree(Prb);
            if (Vars->Grid_Option==BLOCK)    Bin_Probes_Block(Prb);
        }
    }

    void Bin_Probes_Tree(const StateVector &Src);
    void Bin_Probes_Block(const StateVector &Src);

    void Bin_Periodic();

    void        Set_Sources_to_Probes()
    {
        // In this case we can avoid a lot of work by simply transferring the source data to the
        // probe data for the base boxes

        StdAppend(ProActive_Boxes,Active_Boxes);

        OpenMPfor
        for (int i=0; i<ProActive_Boxes.size(); i++)
        {
            SPBox A_Box = ProActive_Boxes[i];
            A_Box->ProActive = true;
            StdAppend(A_Box->Prb_Nodes,A_Box->Src_Nodes);
            StdAppend(A_Box->Prb_ID,A_Box->Src_ID);
        }

        N_Probes = N_Sources;
    }

    //--- Domain Specification

    Cart_ID Min_Bounds(const StateVector &P, const Real &L);
    Cart_ID Max_Bounds(const StateVector &P, const Real &L);
    void    Update_Block_Grid(const StateVector &X, const StateVector &P=Empty_SV);

    void    Specify_Block_Grid(const StateVector &P);
    void    Specify_Block_Grid_Unbounded(const StateVector &P);
    void    Specify_Block_Grid_Periodic(const StateVector &P);
    void    Specify_Block_Grid(const Cart_ID &Lower,const Cart_ID &Upper);
    virtual void    Create_Monolithic_Grid()        {}

    //--- Box retrieval

    SPBoxList   Get_Active_Boxes()            {return Active_Boxes;}
    SPBoxList   Get_ProActive_Boxes()         {return ProActive_Boxes;}
    SPBoxList   Get_Periodic_Boxes()          {return ProActive_Boxes_Periodic;}

    //-------Visualisation--------------

    void Export_Boxes_VTK(const StateVector &V);

    //--------------------------
    //--- Construct Tree
    //--------------------------

    virtual void        Clear_BlockGrid() {}
    virtual void        Clear_Tree() {}

    //--------------------------
    //--- Poisson solver
    //--------------------------

    void        Overlap_Params(SP_EGrid Src, SP_EGrid Dest, const int &OL, Cart_ID &SC,Cart_ID &RC,Cart_ID &W);

    //--- Getters

    Tree_Node<Box> *Get_BoxTree()       {return Box_OctTree;}

    //--- Tree operations
    void            Get_NearField(SPBox B, SPBoxList *NF);
    SPBox           Get_Box_Parent(SPBox Child);
    void            Get_Children(SPBox Parent, SPBoxList *CBoxes);

    //--- Grid functions

    Vector3      Box_Corner(const Cart_ID &CID, const uint &Br=0)
    {
        // Calculates the corner point of the box
        Real SL = H*pow(2,Br);
        return Vector3(CID(0),CID(1),CID(2))*SL + Grid_Vars->Origin;
    }

    Vector3      Box_Centre(const Cart_ID &CID, const uint &Br=0)
    {
        // Calculates the corner point of the box
        Real SL = H*pow(2,Br);
        Vector3 Corn = Vector3(CID(0),CID(1),CID(2))*SL + Grid_Vars->Origin;
        if (Vars->Dim==SIM_3D) return Corn + Vector3(0.5,0.5,0.5)*H;
        if (Vars->Dim==SIM_2D) return Corn + Vector3(0.5,0.5,0)*H;
    }

    ~Box_Tree()
    {
        Active_Boxes.clear();
        ProActive_Boxes.clear();
        ProActive_Boxes_Periodic.clear();
        Boxes.clear();
        delete Box_OctTree;
    }
};

}

#endif // GRID_H
