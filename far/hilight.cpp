/*
hilight.cpp

Files highlighting

*/

#include "headers.hpp"
#pragma hdrstop

#include "colors.hpp"
#include "struct.hpp"
#include "hilight.hpp"
#include "fn.hpp"
#include "global.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "filelist.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"

struct HighlightStrings
{
  wchar_t *UseAttr,*IncludeAttributes,*ExcludeAttributes,*AttrSet,*AttrClear,
          *IgnoreMask,*UseMask,*Mask,
          *NormalColor,*SelectedColor,*CursorColor,*SelectedCursorColor,
          *MarkCharNormalColor,*MarkCharSelectedColor,*MarkCharCursorColor,*MarkCharSelectedCursorColor,
          *MarkChar,
          *ContinueProcessing,
          *UseDate,*DateType,*DateAfter,*DateBefore,
          *UseSize,*SizeType,*SizeAbove,*SizeBelow,
          *HighlightEdit,*HighlightList;
};
static const HighlightStrings HLS=
{
  L"UseAttr",L"IncludeAttributes",L"ExcludeAttributes",L"AttrSet",L"AttrClear",
  L"IgnoreMask",L"UseMask",L"Mask",
  L"NormalColor",L"SelectedColor",L"CursorColor",L"SelectedCursorColor",
  L"MarkCharNormalColor",L"MarkCharSelectedColor",L"MarkCharCursorColor",L"MarkCharSelectedCursorColor",
  L"MarkChar",
  L"ContinueProcessing",
  L"UseDate",L"DateType",L"DateAfter",L"DateBefore",
  L"UseSize",L"SizeType",L"SizeAbove",L"SizeBelow",
  L"HighlightEdit",L"HighlightList"
};

static const wchar_t fmtFirstGroup[]=L"Group%d";
static const wchar_t fmtUpperGroup[]=L"UpperGroup%d";
static const wchar_t fmtLowerGroup[]=L"LowerGroup%d";
static const wchar_t fmtLastGroup[]=L"LastGroup%d";
static const wchar_t SortGroupsKeyName[]=L"SortGroups";

HighlightFiles::HighlightFiles()
{
  InitHighlightFiles();
}

void LoadFilterFromReg(FileFilterParams *HData, const wchar_t *RegKey, const wchar_t *Mask, int SortGroup, bool bSortGroup)
{
  //��������� �������� ������� ��� ���� ��� ����� ���������� ���������
  //��������� ������ ������ ����.

  if (bSortGroup)
    HData->SetMask((DWORD)GetRegKey(RegKey,HLS.UseMask,1), Mask);
  else
    HData->SetMask((DWORD)(GetRegKey(RegKey,HLS.IgnoreMask,0)?0:1), Mask);


  FILETIME DateAfter, DateBefore;
  GetRegKey(RegKey,HLS.DateAfter,(BYTE *)&DateAfter,NULL,sizeof(DateAfter));
  GetRegKey(RegKey,HLS.DateBefore,(BYTE *)&DateBefore,NULL,sizeof(DateBefore));
  HData->SetDate((DWORD)GetRegKey(RegKey,HLS.UseDate,0),
                 (DWORD)GetRegKey(RegKey,HLS.DateType,0),
                 DateAfter,
                 DateBefore);

  HData->SetSize((DWORD)GetRegKey(RegKey,HLS.UseSize,0),
                 (DWORD)GetRegKey(RegKey,HLS.SizeType,0),
                 GetRegKey64(RegKey,HLS.SizeAbove,(unsigned __int64)_i64(-1)),
                 GetRegKey64(RegKey,HLS.SizeBelow,(unsigned __int64)_i64(-1)));

  if (bSortGroup)
  {
    HData->SetAttr((DWORD)GetRegKey(RegKey,HLS.UseAttr,1),
                   (DWORD)GetRegKey(RegKey,HLS.AttrSet,0),
                   (DWORD)GetRegKey(RegKey,HLS.AttrClear,FILE_ATTRIBUTE_DIRECTORY));
  }
  else
  {
    HData->SetAttr((DWORD)GetRegKey(RegKey,HLS.UseAttr,1),
                   (DWORD)GetRegKey(RegKey,HLS.IncludeAttributes,0),
                   (DWORD)GetRegKey(RegKey,HLS.ExcludeAttributes,0));
  }

  HData->SetSortGroup(SortGroup);

  HighlightDataColor Colors;
  Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL]=(WORD)GetRegKey(RegKey,HLS.NormalColor,0);
  Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED]=(WORD)GetRegKey(RegKey,HLS.SelectedColor,0);
  Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR]=(WORD)GetRegKey(RegKey,HLS.CursorColor,0);
  Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]=(WORD)GetRegKey(RegKey,HLS.SelectedCursorColor,0);
  Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_NORMAL]=(WORD)GetRegKey(RegKey,HLS.MarkCharNormalColor,0);
  Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTED]=(WORD)GetRegKey(RegKey,HLS.MarkCharSelectedColor,0);
  Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_UNDERCURSOR]=(WORD)GetRegKey(RegKey,HLS.MarkCharCursorColor,0);
  Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]=(WORD)GetRegKey(RegKey,HLS.MarkCharSelectedCursorColor,0);
  Colors.MarkChar=GetRegKey(RegKey,HLS.MarkChar,0);

  HData->SetColors(&Colors);

  HData->SetContinueProcessing(GetRegKey(RegKey,HLS.ContinueProcessing,0)!=0);
}

void HighlightFiles::InitHighlightFiles()
{
  string strRegKey, strGroupName, strMask;
  const int GroupDelta[4]={DEFAULT_SORT_GROUP,0,DEFAULT_SORT_GROUP+1,DEFAULT_SORT_GROUP};
  const wchar_t *KeyNames[4]={RegColorsHighlight,SortGroupsKeyName,SortGroupsKeyName,RegColorsHighlight};
  const wchar_t *GroupNames[4]={fmtFirstGroup,fmtUpperGroup,fmtLowerGroup,fmtLastGroup};
  int  *Count[4] = {&FirstCount,&UpperCount,&LowerCount,&LastCount};

  HiData.Free();

  FirstCount=UpperCount=LowerCount=LastCount=0;
  for (int j=0; j<4; j++)
  {
    for (int i=0;;i++)
    {
      strGroupName.Format(GroupNames[j],i);
      strRegKey.Format(L"%s\\%s",KeyNames[j],(const wchar_t *)strGroupName);

      if (GroupDelta[j]!=DEFAULT_SORT_GROUP)
      {
        if (!GetRegKey(KeyNames[j],strGroupName,strMask,L""))
          break;
      }
      else
      {
        if (!GetRegKey(strRegKey,HLS.Mask,strMask,L""))
          break;
      }

      FileFilterParams *HData = HiData.addItem();

      if(HData)
      {
        LoadFilterFromReg(HData,strRegKey,strMask,GroupDelta[j]+(GroupDelta[j]==DEFAULT_SORT_GROUP?0:i),(GroupDelta[j]==DEFAULT_SORT_GROUP?false:true));

        (*(Count[j]))++;
      }
      else
        break;
    }
  }
}


HighlightFiles::~HighlightFiles()
{
  ClearData();
}

void HighlightFiles::ClearData()
{
  HiData.Free();
  FirstCount=UpperCount=LowerCount=LastCount=0;
}

static const DWORD FarColor[] = {COL_PANELTEXT,COL_PANELSELECTEDTEXT,COL_PANELCURSOR,COL_PANELSELECTEDCURSOR};

void ApplyDefaultStartingColors(HighlightDataColor *Colors)
{
  for (int j=0; j<2; j++)
    for (int i=0; i<4; i++)
      Colors->Color[j][i]=0xFF00;
  Colors->MarkChar=0x00FF0000;
}

void ApplyBlackOnBlackColors(HighlightDataColor *Colors)
{
  for (int i=0; i<4; i++)
  {
    //�������� black on black.
    //��� ������ ������� ����� ������ �� ������� ������������.
    //��� ������� ������� ����� ����� ������� ������������.

    if (!(Colors->Color[HIGHLIGHTCOLORTYPE_FILE][i]&0x00FF))
      Colors->Color[HIGHLIGHTCOLORTYPE_FILE][i]=(Colors->Color[HIGHLIGHTCOLORTYPE_FILE][i]&0xFF00)|(0x00FF&Palette[FarColor[i]-COL_FIRSTPALETTECOLOR]);

    if (!(Colors->Color[HIGHLIGHTCOLORTYPE_MARKCHAR][i]&0x00FF))
      Colors->Color[HIGHLIGHTCOLORTYPE_MARKCHAR][i]=Colors->Color[HIGHLIGHTCOLORTYPE_FILE][i];
  }
}

void ApplyColors(HighlightDataColor *DestColors, HighlightDataColor *SrcColors)
{
  //���������� black on black ���� ����������� ���������� �����
  //� ���� ����� ������������ ���� ���������� �����.
  ApplyBlackOnBlackColors(DestColors);
  ApplyBlackOnBlackColors(SrcColors);

  for (int j=0; j<2; j++)
  {
    for (int i=0; i<4; i++)
    {
      //���� ������� ����� � Src (fore �/��� back) �� ����������
      //�� ���������� �� � Dest.
      if (!(SrcColors->Color[j][i]&0xF000))
        DestColors->Color[j][i]=(DestColors->Color[j][i]&0x0F0F)|(SrcColors->Color[j][i]&0xF0F0);
      if (!(SrcColors->Color[j][i]&0x0F00))
        DestColors->Color[j][i]=(DestColors->Color[j][i]&0xF0F0)|(SrcColors->Color[j][i]&0x0F0F);
    }
  }

  //���������� ������� �� Src ���� ��� �� ����������
  if (!(SrcColors->MarkChar&0x00FF0000))
    DestColors->MarkChar=SrcColors->MarkChar;
}

/*
bool HasTransparent(HighlightDataColor *Colors)
{
  for (int j=0; j<2; j++)
    for (int i=0; i<4; i++)
      if (Colors->Color[j][i]&0xFF00)
        return true;

  if (Colors->MarkChar&0x00FF0000)
    return true;

  return false;
}
*/

void ApplyFinalColors(HighlightDataColor *Colors)
{
  //���������� black on black ���� ����� ������������ ���� ���������� �����.
  ApplyBlackOnBlackColors(Colors);

  for (int j=0; j<2; j++)
    for (int i=0; i<4; i++)
    {
      //���� ����� �� �� ������� ������ (fore ��� back) ����������
      //�� ���������� ��������������� ���� � �������.
      BYTE temp=(BYTE)((Colors->Color[j][i]&0xFF00)>>8);
      Colors->Color[j][i]=((~temp)&(BYTE)Colors->Color[j][i])|(temp&(BYTE)Palette[FarColor[i]-COL_FIRSTPALETTECOLOR]);
    }

  //���� ������ ������� ���������� �� ��� ��� �� � ��� ������.
  if (Colors->MarkChar&0x00FF0000)
    Colors->MarkChar=0;

  //������� �� �������� �����:
  //���������� black on black ����� ���� ������������ ������������� �����.
  ApplyBlackOnBlackColors(Colors);
}

void HighlightFiles::GetHiColor(const FAR_FIND_DATA *fd,HighlightDataColor *Colors,bool UseAttrHighlighting)
{
  FileFilterParams *CurHiData;

  ApplyDefaultStartingColors(Colors);

  for (unsigned int i=0; i < HiData.getCount(); i++)
  {
    CurHiData = HiData.getItem(i);

    if (UseAttrHighlighting && CurHiData->GetMask(NULL))
      continue;

    if (CurHiData->FileInFilter(fd))
    {
      HighlightDataColor TempColors;
      CurHiData->GetColors(&TempColors);
      ApplyColors(Colors,&TempColors);
      if (!CurHiData->GetContinueProcessing())// || !HasTransparent(Colors))
        break;
    }
  }

  ApplyFinalColors(Colors);
}

void HighlightFiles::GetHiColor(FileListItem **FileItem,int FileCount,bool UseAttrHighlighting)
{
  if(!FileItem || !FileCount)
    return;

  FileFilterParams *CurHiData;
  FileListItem *fli;

  for(int FCnt=0; FCnt < FileCount; ++FCnt)
  {
    fli = FileItem[FCnt];

    ApplyDefaultStartingColors(&fli->Colors);

    for (unsigned int i=0; i < HiData.getCount(); i++)
    {
      CurHiData = HiData.getItem(i);

      if (UseAttrHighlighting && CurHiData->GetMask(NULL))
        continue;

      if (CurHiData->FileInFilter(fli))
      {
        HighlightDataColor TempColors;
        CurHiData->GetColors(&TempColors);
        ApplyColors(&fli->Colors,&TempColors);
        if (!CurHiData->GetContinueProcessing())// || !HasTransparent(&fli->Colors))
          break;
      }
    }

    ApplyFinalColors(&fli->Colors);
  }
}

int HighlightFiles::GetGroup(const FAR_FIND_DATA *fd)
{
  for (int i=FirstCount; i<FirstCount+UpperCount; i++)
  {
    FileFilterParams *CurGroupData=HiData.getItem(i);
    if(CurGroupData->FileInFilter(fd))
       return(CurGroupData->GetSortGroup());
  }

  for (int i=FirstCount+UpperCount; i<FirstCount+UpperCount+LowerCount; i++)
  {
    FileFilterParams *CurGroupData=HiData.getItem(i);
    if(CurGroupData->FileInFilter(fd))
       return(CurGroupData->GetSortGroup());
  }
  return DEFAULT_SORT_GROUP;
}

int HighlightFiles::GetGroup(const FileListItem *fli)
{
  for (int i=FirstCount; i<FirstCount+UpperCount; i++)
  {
    FileFilterParams *CurGroupData=HiData.getItem(i);
    if(CurGroupData->FileInFilter(fli))
       return(CurGroupData->GetSortGroup());
  }

  for (int i=FirstCount+UpperCount; i<FirstCount+UpperCount+LowerCount; i++)
  {
    FileFilterParams *CurGroupData=HiData.getItem(i);
    if(CurGroupData->FileInFilter(fli))
       return(CurGroupData->GetSortGroup());
  }

  return DEFAULT_SORT_GROUP;
}

void HighlightFiles::FillMenu(VMenu *HiMenu,int MenuPos)
{
  MenuItemEx HiMenuItem;
  const int Count[4][2] = {
                            {0,                               FirstCount},
                            {FirstCount,                      FirstCount+UpperCount},
                            {FirstCount+UpperCount,           FirstCount+UpperCount+LowerCount},
                            {FirstCount+UpperCount+LowerCount,FirstCount+UpperCount+LowerCount+LastCount}
                          };

  HiMenu->DeleteItems();
  HiMenuItem.Clear();

  for (int j=0; j<4; j++)
  {
    for (int i=Count[j][0]; i<Count[j][1]; i++)
    {
      MenuString(HiMenuItem.strName,HiData.getItem(i),true);
      HiMenu->AddItem(&HiMenuItem);
    }

    HiMenuItem.strName="";
    HiMenu->AddItem(&HiMenuItem);
    if (j<3)
    {
      if (j==0)
        HiMenuItem.strName = UMSG(MHighlightUpperSortGroup);
      else if (j==1)
        HiMenuItem.strName = UMSG(MHighlightLowerSortGroup);
      else
        HiMenuItem.strName = UMSG(MHighlightLastGroup);

      HiMenuItem.Flags|=LIF_SEPARATOR;
      HiMenu->AddItem(&HiMenuItem);
      HiMenuItem.Flags=0;
    }
  }

  HiMenu->SetSelectPos(MenuPos,1);
}

void HighlightFiles::ProcessGroups()
{
  for (int i=0; i<FirstCount; i++)
    HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP);

  for (int i=FirstCount; i<FirstCount+UpperCount; i++)
    HiData.getItem(i)->SetSortGroup(i-FirstCount);

  for (int i=FirstCount+UpperCount; i<FirstCount+UpperCount+LowerCount; i++)
    HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP+1+i-FirstCount-UpperCount);

  for (int i=FirstCount+UpperCount+LowerCount; i<FirstCount+UpperCount+LowerCount+LastCount; i++)
    HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP);
}

int HighlightFiles::MenuPosToRealPos(int MenuPos, int **Count, bool Insert)
{
  int Pos=MenuPos;
  *Count=NULL;
  int x = Insert ? 1 : 0;

  if (MenuPos<FirstCount+x)
  {
    *Count=&FirstCount;
  }
  else if (MenuPos>FirstCount+1 && MenuPos<FirstCount+UpperCount+2+x)
  {
    Pos=MenuPos-2;
    *Count=&UpperCount;
  }
  else if (MenuPos>FirstCount+UpperCount+3 && MenuPos<FirstCount+UpperCount+LowerCount+4+x)
  {
    Pos=MenuPos-4;
    *Count=&LowerCount;
  }
  else if (MenuPos>FirstCount+UpperCount+LowerCount+5 && MenuPos<FirstCount+UpperCount+LowerCount+LastCount+6+x)
  {
    Pos=MenuPos-6;
    *Count=&LastCount;
  }

  return Pos;
}

void HighlightFiles::HiEdit(int MenuPos)
{
  VMenu HiMenu(UMSG(MHighlightTitle),NULL,0,ScrY-4);
  HiMenu.SetHelp(HLS.HighlightList);
  HiMenu.SetFlags(VMENU_WRAPMODE|VMENU_SHOWAMPERSAND);
  HiMenu.SetPosition(-1,-1,0,0);
  HiMenu.SetBottomTitle(UMSG(MHighlightBottom));

  FillMenu(&HiMenu,MenuPos);

  int NeedUpdate;
  Panel *LeftPanel=CtrlObject->Cp()->LeftPanel;
  Panel *RightPanel=CtrlObject->Cp()->RightPanel;

  HiMenu.Show();
  while (1)
  {
    while (!HiMenu.Done())
    {
      int SelectPos=HiMenu.GetSelectPos();
      int Key;

      NeedUpdate=FALSE;
      Key=HiMenu.ReadInput();
      switch(Key)
      {
        /* $ 07.07.2000 IS
          ���� ������ ctrl+r, �� ������������ �������� �� ���������.
        */
        case KEY_CTRLR:
          if (Message(MSG_WARNING,2,UMSG(MHighlightTitle),
                        UMSG(MHighlightWarning),UMSG(MHighlightAskRestore),
                        UMSG(MYes),UMSG(MCancel))!=0)
             break;
          DeleteKeyTree(RegColorsHighlight);
          SetHighlighting();
          HiMenu.Hide();
          ClearData();
          InitHighlightFiles();
          NeedUpdate=TRUE;
          break;
        case KEY_NUMDEL:
        case KEY_DEL:
          {
            int *Count=NULL;
            int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);
            if (Count && RealSelectPos<(int)HiData.getCount())
            {
              const wchar_t *Mask;
              HiData.getItem(RealSelectPos)->GetMask(&Mask);
              if (Message(MSG_WARNING,2,UMSG(MHighlightTitle),
                          UMSG(MHighlightAskDel),Mask,
                          UMSG(MDelete),UMSG(MCancel))!=0)
                break;

              HiData.deleteItem(RealSelectPos);
              (*Count)--;
              NeedUpdate=TRUE;
            }
            break;
          }
        case KEY_NUMENTER:
        case KEY_ENTER:
        case KEY_F4:
        {
          int *Count=NULL;
          int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);
          if (Count && RealSelectPos<(int)HiData.getCount())
            if (FileFilterConfig(HiData.getItem(RealSelectPos),true))
              NeedUpdate=TRUE;
          break;
        }
        case KEY_INS: case KEY_NUMPAD0:
        {
          int *Count=NULL;
          int RealSelectPos=MenuPosToRealPos(SelectPos,&Count,true);
          if (Count)
          {
            FileFilterParams *NewHData = HiData.insertItem(RealSelectPos);

            if (!NewHData)
              break;

            if (FileFilterConfig(NewHData,true))
            {
              (*Count)++;
              NeedUpdate=TRUE;
            }
            else
              HiData.deleteItem(RealSelectPos);
          }
          break;
        }
        case KEY_F5:
          {
            int *Count=NULL;
            int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);
            if (Count && RealSelectPos<(int)HiData.getCount())
            {
              FileFilterParams *HData = HiData.insertItem(RealSelectPos);

              if (HData)
              {
                *HData = *HiData.getItem(RealSelectPos+1);
                HData->SetTitle(L"");
                if (FileFilterConfig(HData,true))
                {
                  NeedUpdate=TRUE;
                  (*Count)++;
                }
                else
                  HiData.deleteItem(RealSelectPos);
              }
            }
            break;
          }
        case KEY_CTRLUP: case KEY_CTRLNUMPAD8:
          {
            int *Count=NULL;
            int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);
            if (Count && SelectPos > 0)
            {
              if (UpperCount && RealSelectPos==FirstCount && RealSelectPos<FirstCount+UpperCount)
              {
                FirstCount++;
                UpperCount--;
                SelectPos--;
              }
              else if (LowerCount && RealSelectPos==FirstCount+UpperCount && RealSelectPos<FirstCount+UpperCount+LowerCount)
              {
                UpperCount++;
                LowerCount--;
                SelectPos--;
              }
              else if (LastCount && RealSelectPos==FirstCount+UpperCount+LowerCount)
              {
                LowerCount++;
                LastCount--;
                SelectPos--;
              }
              else
                HiData.swapItems(RealSelectPos,RealSelectPos-1);
              HiMenu.SetSelection(--SelectPos);
              NeedUpdate=TRUE;
              break;
            }
            HiMenu.ProcessInput();
            break;
          }
        case KEY_CTRLDOWN: case KEY_CTRLNUMPAD2:
          {
            int *Count=NULL;
            int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);
            if (Count && SelectPos < (int)HiMenu.GetItemCount()-2)
            {
              if (FirstCount && RealSelectPos==FirstCount-1)
              {
                FirstCount--;
                UpperCount++;
                SelectPos++;
              }
              else if (UpperCount && RealSelectPos==FirstCount+UpperCount-1)
              {
                UpperCount--;
                LowerCount++;
                SelectPos++;
              }
              else if (LowerCount && RealSelectPos==FirstCount+UpperCount+LowerCount-1)
              {
                LowerCount--;
                LastCount++;
                SelectPos++;
              }
              else
                HiData.swapItems(RealSelectPos,RealSelectPos+1);
              HiMenu.SetSelection(++SelectPos);
              NeedUpdate=TRUE;
            }
            HiMenu.ProcessInput();
            break;
          }

        default:
          HiMenu.ProcessInput();
          break;
      }
      // ������������� �����!
      if(NeedUpdate)
      {
        ScrBuf.Lock(); // �������� ������ ����������
        HiMenu.Hide();

        ProcessGroups();

        if(Opt.AutoSaveSetup)
          SaveHiData();
        //FrameManager->RefreshFrame(); // ��������

        LeftPanel->Update(UPDATE_KEEP_SELECTION);
        LeftPanel->Redraw();
        RightPanel->Update(UPDATE_KEEP_SELECTION);
        RightPanel->Redraw();

        FillMenu(&HiMenu,MenuPos=SelectPos);
		HiMenu.SetPosition(-1,-1,0,0);
        HiMenu.Show();
        ScrBuf.Unlock(); // ��������� ����������
      }
    }
    if (HiMenu.Modal::GetExitCode()!=-1)
    {
      HiMenu.ClearDone();
      HiMenu.WriteInput(KEY_F4);
      continue;
    }
    break;
  }
}

void SaveFilterToReg(FileFilterParams *CurHiData, const wchar_t *RegKey, bool bSortGroup)
{
  if (bSortGroup)
    SetRegKey(RegKey,HLS.UseMask,CurHiData->GetMask(NULL));
  else
  {
    const wchar_t *Mask;
    SetRegKey(RegKey,HLS.IgnoreMask,(CurHiData->GetMask(&Mask) ? 0 : 1));
    SetRegKey(RegKey,HLS.Mask,Mask);
  }

  DWORD DateType;
  FILETIME DateAfter, DateBefore;
  SetRegKey(RegKey,HLS.UseDate,CurHiData->GetDate(&DateType, &DateAfter, &DateBefore));
  SetRegKey(RegKey,HLS.DateType,DateType);
  SetRegKey(RegKey,HLS.DateAfter,(BYTE *)&DateAfter,sizeof(DateAfter));
  SetRegKey(RegKey,HLS.DateBefore,(BYTE *)&DateBefore,sizeof(DateBefore));

  DWORD SizeType;
  __int64 SizeAbove, SizeBelow;
  SetRegKey(RegKey,HLS.UseSize,CurHiData->GetSize(&SizeType, &SizeAbove, &SizeBelow));
  SetRegKey(RegKey,HLS.SizeType,SizeType);
  SetRegKey64(RegKey,HLS.SizeAbove,SizeAbove);
  SetRegKey64(RegKey,HLS.SizeBelow,SizeBelow);

  DWORD AttrSet, AttrClear;
  SetRegKey(RegKey,HLS.UseAttr,CurHiData->GetAttr(&AttrSet, &AttrClear));
  SetRegKey(RegKey,(bSortGroup?HLS.AttrSet:HLS.IncludeAttributes),AttrSet);
  SetRegKey(RegKey,(bSortGroup?HLS.AttrClear:HLS.ExcludeAttributes),AttrClear);

  HighlightDataColor Colors;
  CurHiData->GetColors(&Colors);
  SetRegKey(RegKey,HLS.NormalColor,(DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL]);
  SetRegKey(RegKey,HLS.SelectedColor,(DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED]);
  SetRegKey(RegKey,HLS.CursorColor,(DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR]);
  SetRegKey(RegKey,HLS.SelectedCursorColor,(DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]);
  SetRegKey(RegKey,HLS.MarkCharNormalColor,(DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_NORMAL]);
  SetRegKey(RegKey,HLS.MarkCharSelectedColor,(DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTED]);
  SetRegKey(RegKey,HLS.MarkCharCursorColor,(DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_UNDERCURSOR]);
  SetRegKey(RegKey,HLS.MarkCharSelectedCursorColor,(DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]);
  SetRegKey(RegKey,HLS.MarkChar,Colors.MarkChar);

  SetRegKey(RegKey,HLS.ContinueProcessing,(CurHiData->GetContinueProcessing() ? 1 : 0));
}

void HighlightFiles::SaveHiData()
{
  string strRegKey, strGroupName;
  const wchar_t *KeyNames[4]={RegColorsHighlight,SortGroupsKeyName,SortGroupsKeyName,RegColorsHighlight};
  const wchar_t *GroupNames[4]={fmtFirstGroup,fmtUpperGroup,fmtLowerGroup,fmtLastGroup};
  const int Count[4][2] = {
                            {0,                               FirstCount},
                            {FirstCount,                      FirstCount+UpperCount},
                            {FirstCount+UpperCount,           FirstCount+UpperCount+LowerCount},
                            {FirstCount+UpperCount+LowerCount,FirstCount+UpperCount+LowerCount+LastCount}
                          };

  for (int j=0; j<4; j++)
  {
    for (int i=Count[j][0]; i<Count[j][1]; i++)
    {
      strGroupName.Format(GroupNames[j],i-Count[j][0]);
      strRegKey.Format(L"%s\\%s",KeyNames[j],(const wchar_t *)strGroupName);

      FileFilterParams *CurHiData=HiData.getItem(i);

      if (j!=0 && j!=3)
      {
        const wchar_t *Mask;
        CurHiData->GetMask(&Mask);
        SetRegKey(KeyNames[j],strGroupName,Mask);
      }

      SaveFilterToReg(CurHiData,strRegKey,(j==0 || j==3?false:true));
    }

    for (int i=0; i<5; i++)
    {
      strGroupName.Format(GroupNames[j],Count[j][1]-Count[j][0]+i);
      strRegKey.Format(L"%s\\%s",KeyNames[j],(const wchar_t *)strGroupName);
      if (j!=0 && j!=3)
        DeleteRegValue(KeyNames[j],strGroupName);
      DeleteRegKey(strRegKey);
    }
  }
}

void SetHighlighting()
{
  if (CheckRegKey(RegColorsHighlight))
    return;

  string strRegKey;
  // ����� �������� %PATHEXT%, � FileFilterParams::SetMask() ��� �����������
  // �������.
  string strCmdExt = L"*.exe,*.com,*.bat,%PATHEXT%";
  static const wchar_t *Masks[]={
  /* 0 */ L"*.*",
  /* 1 */ L"*.rar,*.zip,*.[zj],*.[bg7]z,*.[bg]zip,*.tar,*.t[ag]z,*.ar[cj],*.r[0-9][0-9],*.a[0-9][0-9],*.bz2,*.cab,*.msi,*.jar,*.lha,*.lzh,*.ha,*.ac[bei],*.pa[ck],*.rk,*.cpio,*.rpm,*.zoo,*.hqx,*.sit,*.ice,*.uc2,*.ain,*.imp,*.777,*.ufa,*.boa,*.bs[2a],*.sea,*.hpk,*.ddi,*.x2,*.rkv,*.[lw]sz,*.h[ay]p,*.lim,*.sqz,*.chz",
  /* 2 */ L"*.bak,*.tmp",                                                                                                                                                                                //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ -> ����� � ���������? ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
          /* $ 25.09.2001  IS
              ��� ����� ��� ���������: ������������ ��� ��������, ����� ���, ���
              �������� ������������� (�� ����� - ��� �����).
          */
  /* 3 */ L"*.*|..", // ����� ��� ���������
  /* 4 */ L"..",     // ����� �������� ���������� ��� ������� �����
  };

  static struct DefaultData
  {
    const wchar_t *Mask;
    int IgnoreMask;
    DWORD IncludeAttr;
    BYTE NormalColor;
    BYTE CursorColor;
  }
  StdHighlightData[]=
  { /*
             Mask                NormalColor
                          IncludeAttributes
                       IgnoreMask       CursorColor             */
     /* 0 */{Masks[0], 0, 0x0002, 0x13, 0x38},
     /* 1 */{Masks[0], 0, 0x0004, 0x13, 0x38},
     /* 2 */{Masks[3], 0, 0x0010, 0x1F, 0x3F},
     /* 3 */{Masks[4], 0, 0x0010, 0x00, 0x00},
     /* 4 */{strCmdExt,0, 0x0000, 0x1A, 0x3A},
     /* 5 */{Masks[1], 0, 0x0000, 0x1D, 0x3D},
     /* 6 */{Masks[2], 0, 0x0000, 0x16, 0x36},
            // ��� ��������� ��� ��������� �� ��� �������, ������� ������ ��������������
            // ��� ����� ����� (��������, ������ ������ � "far navigator")
     /* 7 */{Masks[0], 1, 0x0010, 0x1F, 0x3F},
  };

  // ��� NT ��������� CMD
  if(WinVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
    strCmdExt+=L",*.cmd";

  for(int I=0; I < countof(StdHighlightData); I++)
  {
  	strRegKey.Format(L"%s\\Group%d",RegColorsHighlight,I);
    SetRegKey(strRegKey,HLS.Mask,StdHighlightData[I].Mask);
    SetRegKey(strRegKey,HLS.IgnoreMask,StdHighlightData[I].IgnoreMask);
    SetRegKey(strRegKey,HLS.IncludeAttributes,StdHighlightData[I].IncludeAttr);
    SetRegKey(strRegKey,HLS.NormalColor,StdHighlightData[I].NormalColor);
    SetRegKey(strRegKey,HLS.CursorColor,StdHighlightData[I].CursorColor);
  }
}
