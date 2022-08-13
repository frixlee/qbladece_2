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

//-----------------------------------------------------------------------------
//-------------------------ML Domain Class Functions---------------------------
//-----------------------------------------------------------------------------

#include "Box_Tree.h"
#include "VPML_Gobal_Vars.h"
//#include "Kernels.cpp"
#include "Particle.h"
#include "Tree_Methods.h"

namespace VPML
{

//-----------------------------------
//--------TREE STYLE GRID------------
//-----------------------------------

//--- Source and Probe Binning

void Box_Tree::Bin_Sources_Tree(const StateVector &Src)
{
    // This function is written to avoid the necessity to store the Nodes somewhere else
    // other than the boxes. The boxes are identified and created if necessary

    if (Src.empty())    return;
    LOG_FUNC_ENTRY;

    // Specify Cartesian ID
    std::vector<Cart_ID> CIDSTemp;
    CIDSTemp.assign(Src.size(),Cart_ID::Zero());
    OpenMPfor
    for (int i=0; i<Src.size(); i++)    CIDSTemp[i] = OctTree_CID(RelPos(Src[i]),H);

    // Specify Tree IDs
    std::vector<TreeID> TIDSTemp;
    TIDSTemp.assign(Src.size(),TreeTemp);
    OpenMPfor
    for (int i=0; i<Src.size(); i++)    OctTree_TID(CIDSTemp[i],TIDSTemp[i]);

    // Retrieve box. Add source to box. Add box to active boxes.
    // Tree is not thread-safe. This must be carried out in parallel.
    for (int i=0; i<Src.size(); i++)
    {
        SPBox B = Get_Box(CIDSTemp[i],TIDSTemp[i]);
        if (!B->Active)                             // Set activity
        {
            B->Active = true;
            Active_Boxes.push_back(B);
        }
        B->Add_Source(Src[i],i);
    }

    N_Sources = Src.size();
}

void Box_Tree::Bin_Probes_Tree(const StateVector &Prb)
{
    // This function is written to avoid the necessity to store the Nodes somewhere else
    // other than the boxes. The boxes are identified and created if necessary
    if (Prb.empty())    return;
    LOG_FUNC_ENTRY;

    // Specify Cartesian ID
    std::vector<Cart_ID> CIDSTemp;
    CIDSTemp.assign(Prb.size(),Cart_ID::Zero());
    OpenMPfor
    for (int i=0; i<Prb.size(); i++)    CIDSTemp[i] = OctTree_CID(RelPos(Prb[i]),H);

    // Specify Tree IDs
    std::vector<TreeID> TIDSTemp;
    TIDSTemp.assign(Prb.size(),TreeTemp);
    OpenMPfor
    for (int i=0; i<Prb.size(); i++)    OctTree_TID(CIDSTemp[i],TIDSTemp[i]);

    // Retrieve box. Add source to box. Add box to proactive boxes.
    for (int i=0; i<Prb.size(); i++)
    {
        SPBox B = Get_Box(CIDSTemp[i],TIDSTemp[i]);
        if (!B->ProActive)                  // Set proactivity
        {
            B->ProActive = true;
            ProActive_Boxes.push_back(B);
        }
        B->Add_Probe(Prb[i],i);
    }

    N_Probes = Prb.size();
}

void Box_Tree::Bin_Periodic()
{
    // In the case that the simulation is periodic, we need to declare the regions around both sides
    // of the active area as proactive, such that the far-field effects are accounted for.

    // Declare probe boxes which will later be necessary for periodic problems
    // The far field influence will be calculated with these boxes

    SPBoxList PerBox;       // The boxes which need to be repeated in periodic direction
    Get_Repeated_Boxes(PerBox);

    for (int i=0; i<PerBox.size(); i++)
    {
        Cart_ID CID = PerBox[i]->Get_BoxCID();
        for (int i=1; i<Vars->N_Per_Mirr+1; i++)
        {
            SPBox BU = Get_Box(Cart_ID(CID(0)-Vars->NB_Periodic*i,CID(1),CID(2)));
            ProActive_Boxes_Periodic.push_back(BU);
            SPBox BD = Get_Box(Cart_ID(CID(0)+Vars->NB_Periodic*i,CID(1),CID(2)));
            ProActive_Boxes_Periodic.push_back(BD);
        }
    }
}

//--- Tree operations

void Box_Tree::Get_NearField(SPBox B, SPBoxList *NF)
{
    // This algorithm uses the tree to extract the near field of this particular box

    Cart_ID CID = B->Get_BoxCID();
    int Br = B->Get_Branch();
    int NFC = Grid_Vars->NF_Crit;

    if (Vars->Dim==SIM_2D)
    {

        int IDX = CID(0);
        int IDY = CID(1);

        for(int i=-NFC; i<2*NFC; i++)
        {
            for(int j=-NFC; j<2*NFC; j++)
            {
                //                int IDN[4] = {IDB, IDX+i, IDY+j, 0};
                //                NF->push_back(Get_Box(IDN));

                NF->push_back(Get_Box(Cart_ID(IDX+i, IDY+j,0),Br));
            }
        }
    }

    if (Vars->Dim==SIM_3D)
    {
        int IDX = CID(0);
        int IDY = CID(1);
        int IDZ = CID(2);

        for(int i=-NFC; i<2*NFC; i++)
        {
            for(int j=-NFC; j<2*NFC; j++)
            {
                for(int k=-NFC; k<2*NFC; k++)
                {
                    //                    int IDN[4] = {IDB, IDX+i, IDY+j, IDZ+k};
                    //                    NF->push_back(Get_Box(IDN));
                    //                    NF->push_back(Get_Box(Cart_ID(IDX+i, IDY+j,IDZ+k)));
                    NF->push_back(Get_Box(Cart_ID(IDX+i, IDY+j, IDZ+k),Br));
                }
            }
        }
    }
}

SPBox Box_Tree::Get_Box_Parent(SPBox Child)
{
    // Simply extracts the parent box based on CID (simpler than tree)
    Cart_ID CCID = Child->Get_BoxCID();
    Cart_ID PCID;

    if  (CCID(0)<0) PCID(0) = (CCID(0)-1)/2;
    else            PCID(0) = CCID(0)/2;

    if  (CCID(1)<0) PCID(1) = (CCID(1)-1)/2;
    else            PCID(1) = CCID(1)/2;

    if  (CCID(2)<0) PCID(2) = (CCID(2)-1)/2;
    else            PCID(2) = CCID(2)/2;

    return Get_Box(PCID,Child->Get_Branch()+1);
}

void Box_Tree::Get_Children(SPBox Parent, SPBoxList *CBoxes)
{
    //    Tree_ID RIDC = *Parent->Get_ID();                  // Temporarily copy TreeID
    //    OctBranch<ML_Box> *Br = Box_Tree->Retrieve_Branch(&RIDC);
    //    Br->Get_Children_Objects(CBoxes);

//    int TreeLvl = Tree_Lim-Parent->Get_Branch();
    int Branch = Parent->Get_Branch();
    Tree_Node<Box> *Leaf = Box_OctTree->Get_Leaf(Parent->Get_TID(),Tree_Lim-Branch);
    std::vector<Tree_Node<Box>*> CLeaves;
    Leaf->Get_Sub_Nodes(CLeaves);
    for (int i=0; i<CLeaves.size(); i++)
    {
        if (CLeaves[i]->Get_Object() != nullptr) CBoxes->push_back(CLeaves[i]->Get_Object());
    }
}

//-----------------------------------
//--------BLOCK STYLE GRID-----------
//-----------------------------------

//--- Source and Probe Binning

void Box_Tree::Set_Block_Index(const Vector &V, Cart_ID4 &QInf)
{
    // This is a helper function to specify which base box is inhabited by the source point
    // in case we are using the block formulation of the grid

    Vector3 Pos = Vector3(V(0), V(1), V(2)) - Grid_Vars->Origin;
    Real I = Pos(0)/Grid_Vars->H_ML;
    Real J = Pos(1)/Grid_Vars->H_ML;
    Real K = Pos(2)/Grid_Vars->H_ML;

    int II = int(I);
    int JJ = int(J);
    int KK = int(K);

    if (I<0) II--;
    if (J<0) JJ--;
    if (K<0) KK--;

    QInf(0) = II - Block_Grid_Lower(0);
    QInf(1) = JJ - Block_Grid_Lower(1);
    QInf(2) = KK - Block_Grid_Lower(2);

    if (QInf(0)<0)                      QInf(0)=0;
    if (QInf(0)>Block_Grid_Delta(0)-1)  QInf(0)=Block_Grid_Delta(0)-1;

//        int QI = QInf(0)/QuadFac;
//        int QJ = QInf(1)/QuadFac;
//        int QK = QInf(2)/QuadFac;
//        QInf(3) =  QI*NQY*NQZ + QJ*NQZ + QK;

    QInf(3) = QInf(0)*Block_Grid_Delta(1)*Block_Grid_Delta(2) + QInf(1)*Block_Grid_Delta(2) + QInf(2);

    if (QInf(0)<0) qDebug() << "Error Set_Block_Index. IDX < 0";
    if (QInf(1)<0) qDebug() << "Error Set_Block_Index. IDY < 0";
    if (QInf(2)<0) qDebug() << "Error Set_Block_Index. IDZ < 0";
    if (QInf(0)>Block_Grid_Delta(0)-1) qDebug() << "Error Set_Block_Index. IDX > MAX";
    if (QInf(1)>Block_Grid_Delta(1)-1) qDebug() << "Error Set_Block_Index. IDY > MAX";
    if (QInf(2)>Block_Grid_Delta(2)-1) qDebug() << "Error Set_Block_Index. IDZ > MAX";
    if (QInf(3)>=Block_Grid.size())
    {
        qDebug() << "Error Set_Block_Index. Block grid out of bounds.";
        qDebug() << Pos(0) << Pos(1) << Pos(2) << Block_Grid_Delta(0) << QInf(2) << Vars->Lx_Per;
    }
}

void Box_Tree::Bin_Sources_Block(const StateVector &Src)
{
    // If the monolithic solver is being used, the base boxes are defined on a regular grid.
    // The grid has been defined earlier, this saves time significantly in the binning procedure

    if (Src.empty())    return;
    LOG_FUNC_ENTRY;

    Update_Block_Grid(Src);          //--- Update grid if necessary

    std::vector<Cart_ID4> Binfo;
    Binfo.assign(Src.size(),Cart_ID4::Zero());

    OpenMPfor
    for (int i=0; i<Src.size(); i++) Set_Block_Index(Src[i],Binfo[i]);

    for (int i=0; i<Src.size(); i++)
    {
//        qDebug() << Binfo[i](0) << Binfo[i](1) << Binfo[i](2) << Binfo[i](3) << Block_Grid.size();
        Block_Grid[Binfo[i](3)]->Src_Nodes.push_back(Src[i]);
        Block_Grid[Binfo[i](3)]->Src_ID.push_back(i);
    }

    for (int i=0; i<Block_Grid.size(); i++)
    {
        if (Block_Grid[i]->Src_Nodes.empty()) continue;       // Source nodes empty
        if (Block_Grid[i]->Active)            continue;       // Already set active (eg vol src)
        Block_Grid[i]->Active = true;
        Active_Boxes.push_back(Block_Grid[i]);
    }

    N_Sources = Src.size();
}

void Box_Tree::Bin_Probes_Block(const StateVector &Prb)
{
    // This function is written to avoid the necessity to store the Nodes somewhere else
    // other than the boxes. The boxes are identified and created if necessary

    if (Prb.empty())    return;
    LOG_FUNC_ENTRY;

    Update_Block_Grid(Prb);          //--- Update grid if necessary

    std::vector<Cart_ID4> Binfo;
    Binfo.assign(Prb.size(),Cart_ID4::Zero());

    OpenMPfor
    for (int i=0; i<Prb.size(); i++) Set_Block_Index(Prb[i],Binfo[i]);

    for (int i=0; i<Prb.size(); i++)
    {
        Block_Grid[Binfo[i](3)]->Prb_Nodes.push_back(Prb[i]);
        Block_Grid[Binfo[i](3)]->Prb_ID.push_back(i);
    }

    for (int i=0; i<Block_Grid.size(); i++)
    {
        if (Block_Grid[i]->Prb_Nodes.empty()) continue;       // Source nodes empty
        if (Block_Grid[i]->ProActive)         continue;       // Already set proactive (eg vol src)
        Block_Grid[i]->ProActive = true;
        ProActive_Boxes.push_back(Block_Grid[i]);
    }

    N_Probes = Prb.size();
}

//--------Domain Specification------------------

Cart_ID Box_Tree::Min_Bounds(const StateVector &P, const Real &L)
{
    Real MinXVal = MinX(P);
    Real MinYVal = MinY(P);
    Real MinZVal = MinZ(P);

    int MinX = int(MinXVal/L);
    int MinY = int(MinYVal/L);
    int MinZ = int(MinZVal/L);

    if (MinXVal<0)  MinX--;
    if (MinYVal<0)  MinY--;
    if (MinZVal<0)  MinZ--;

    return Cart_ID(MinX,MinY,MinZ);
}

Cart_ID Box_Tree::Max_Bounds(const StateVector &P, const Real &L)
{
    Real MaxXVal = MaxX(P);
    Real MaxYVal = MaxY(P);
    Real MaxZVal = MaxZ(P);

    int MaxX = int(MaxXVal/L);
    int MaxY = int(MaxYVal/L);
    int MaxZ = int(MaxZVal/L);

    if (MaxXVal<0)  MaxX--;
    if (MaxYVal<0)  MaxY--;
    if (MaxZVal<0)  MaxZ--;

    return Cart_ID(MaxX,MaxY,MaxZ);
}

void Box_Tree::Update_Block_Grid(const StateVector &X, const StateVector &P)
{
    // This ensures the block grid is large enough
    StateVector FullGrid;
    StdAppend(FullGrid,X);
    if (!Vars->Prbs_Are_Srcs) StdAppend(FullGrid,P);
    Specify_Block_Grid(FullGrid);
}

void Box_Tree::Specify_Block_Grid(const StateVector &P)
{
    if (Vars->Periodic) Specify_Block_Grid_Periodic(P);
    else                Specify_Block_Grid_Unbounded(P);        // Only really practical for PML.. GML
}

void Box_Tree::Specify_Block_Grid_Unbounded(const StateVector &P)
{
    // This function inspects the particle set in P.
    // This then creates a block grid based on these values.

    Cart_ID Min_BD = Min_Bounds(P,Grid_Vars->H_ML);
    Cart_ID Max_BD = Max_Bounds(P,Grid_Vars->H_ML);

    // Specify the desired grid
    Cart_ID Lower, Upper;
    Lower(0) = Min_BD(0) - Grid_Vars->Block_Grid_Buffer;
    Lower(1) = Min_BD(1) - Grid_Vars->Block_Grid_Buffer;
    Lower(2) = Min_BD(2) - Grid_Vars->Block_Grid_Buffer;

    Upper(0) = Max_BD(0) + Grid_Vars->Block_Grid_Buffer;
    Upper(1) = Max_BD(1) + Grid_Vars->Block_Grid_Buffer;
    Upper(2) = Max_BD(2) + Grid_Vars->Block_Grid_Buffer;

    // Now check
    bool Create = false;
    if (Block_Grid.empty())             Create = true;
    if (Lower(0)<Block_Grid_Lower(0))   Create = true;
    if (Lower(1)<Block_Grid_Lower(1))   Create = true;
    if (Lower(2)<Block_Grid_Lower(2))   Create = true;
    if (Upper(0)>Block_Grid_Upper(0))   Create = true;
    if (Upper(1)>Block_Grid_Upper(1))   Create = true;
    if (Upper(2)>Block_Grid_Upper(2))   Create = true;

    // If desired, we shall create the boxes and specify
    if (Create)     Specify_Block_Grid(Lower, Upper);
}

void Box_Tree::Specify_Block_Grid_Periodic(const StateVector &P)
{
    // This function inspects the particle set in P.
    // This then creates a block grid based on these values.

    Cart_ID Min_BD = Min_Bounds(P,Grid_Vars->H_ML);
    Cart_ID Max_BD = Max_Bounds(P,Grid_Vars->H_ML);

    // Specify the desired grid
    Cart_ID Lower, Upper;
    Lower(0) = 0;
    Lower(1) = Min_BD(1) - Grid_Vars->Block_Grid_Buffer;
    Lower(2) = Min_BD(2) - Grid_Vars->Block_Grid_Buffer;

    Upper(0) = Vars->NB_Periodic-1;
    Upper(1) = Max_BD(1) + Grid_Vars->Block_Grid_Buffer;
    Upper(2) = Max_BD(2) + Grid_Vars->Block_Grid_Buffer;

    // Now check
    bool Create = false;
    if (Block_Grid.empty())             Create = true;
    if (Lower(1)<Block_Grid_Lower(1))   Create = true;
    if (Lower(2)<Block_Grid_Lower(2))   Create = true;
    if (Upper(1)>Block_Grid_Upper(1))   Create = true;
    if (Upper(2)>Block_Grid_Upper(2))   Create = true;

    // If desired, we shall create the boxes and specify
    if (Create)     Specify_Block_Grid(Lower, Upper);
}

void Box_Tree::Specify_Block_Grid(const Cart_ID &Lower,const Cart_ID &Upper)
{
    // This function inspects the particle set in P.
    // This then creates a block grid based on these values.

    Block_Grid.clear();
    for (int i=Lower(0); i<=Upper(0); i++)
    {
        for (int j=Lower(1); j<=Upper(1); j++)
        {
            for (int k=Lower(2); k<=Upper(2); k++)
            {
                Block_Grid.push_back(Get_Box(Cart_ID(i,j,k)));
            }
        }
    }
    Block_Grid_Lower = Lower;
    Block_Grid_Upper = Upper;
    Block_Grid_Delta = Upper-Lower+Cart_ID(1,1,1);
    Grid_Vars->NBlMono = Block_Grid_Delta;
    Vector3 CornPos = Vector3(Block_Grid_Lower(0), Block_Grid_Lower(1), Block_Grid_Lower(2));
    Block_Corn = Grid_Vars->Origin + Grid_Vars->H_ML*CornPos;
    Vector3 DG = Vector3(Block_Grid_Delta(0), Block_Grid_Delta(1), Block_Grid_Delta(2));
    Vector3 BlockUpperCorn = Block_Corn + Grid_Vars->H_ML*DG;
    qDebug() <<  "Block grid box limits: " << Lower(0) << Lower(1) << Lower(2) << Upper(0) << Upper(1) << Upper(2);

    // A new block grid has been created, the Poisson grid must also be updated

    Create_Monolithic_Grid();
}

//------- Clear grid

void Box_Tree::Clear_Grid()
{
    // This cycles across data stored in the boxes

    OpenMPfor
    for (int i=0; i<Active_Boxes.size(); i++)               Active_Boxes[i]->Clear_Source_Data();
    Active_Boxes.clear();

    OpenMPfor
    for (int i=0; i<ProActive_Boxes.size(); i++)            ProActive_Boxes[i]->Clear_Probe_Data();
    ProActive_Boxes.clear();

    OpenMPfor
    for (int i=0; i<ProActive_Boxes_Periodic.size(); i++)   ProActive_Boxes_Periodic[i]->Clear_Probe_Data();
    ProActive_Boxes_Periodic.clear();

    Clear_BlockGrid();
    Clear_Tree();
}

//---------------- Mapping to and from Eulerian Grids -------------------------

void Box_Tree::Overlap_Params(SP_EGrid Src, SP_EGrid Dest, const int &OL, Cart_ID &SC,Cart_ID &RC,Cart_ID &W)
{

    // This specifies the overlap of the domains when we are tranferring values.
    // This occurs in both tranfer omega and transfer Psi0 sweeps.

    Cart_ID DestBoxID = Dest->Get_CID();
    Cart_ID SrcBoxID = Src->Get_CID();
    Cart_ID DID = SrcBoxID - DestBoxID;

    if (Vars->Periodic){            // Additional flag for periodic case...
        if (DID(0)==(Vars->NB_Periodic-1))  DID(0)=-1;
        if (DID(0)==(1-Vars->NB_Periodic))  DID(0)=1;
    }

    int WX = 2*OL;
    int WY = 2*OL;
    int WZ = 2*OL;

    // Specify overlap sections
    switch (DID(0))
    {
    case (-1):
    {
        SC(0) = Src->nx-WX;
        RC(0) = 0;
        W(0) = WX+1;
        break;
    }
    case (0):
    {
        SC(0) = 0;
        RC(0) = 0;
        W(0) = Src->nx+1;
        break;
    }
    case (1):
    {
        SC(0) = 0;
        RC(0) = Dest->nx-WX;
        W(0) = WX+1;
        break;
    }
    default: break;
    }

    switch (DID(1))
    {
    case (-1):
    {
        SC(1) = Src->ny-WY;
        RC(1) = 0;
        W(1) = WY+1;
        break;
    }
    case (0):
    {
        SC(1) = 0;
        RC(1) = 0;
        W(1) = Src->ny+1;
        break;
    }
    case (1):
    {
        SC(1) = 0;
        RC(1) = Dest->ny-WY;
        W(1) = WY+1;
        break;
    }
    default: break;
    }

    if (Vars->Dim==SIM_2D)  return;

    switch (DID(2))
    {
    case (-1):
    {
        SC(2) = Src->nz-WZ;
        RC(2) = 0;
        W(2) = WZ+1;
        break;
    }
    case (0):
    {
        SC(2) = 0;
        RC(2) = 0;
        W(2) = Src->nz+1;
        break;
    }
    case (1):
    {
        SC(2) = 0;
        RC(2) = Dest->nz-WZ;
        W(2) = WZ+1;
        break;
    }
    default: break;
    }

}

}
