/////////////////////////////////////////////////////////////////////////////
// Program:     wxWidgets Widgets Sample
// Name:        dirctrl.cpp
// Purpose:     Part of the widgets sample showing wxGenericDirCtrl
// Author:      Wlodzimierz 'ABX' Skiba
// Created:     4 Oct 2006
// Copyright:   (c) 2006 wxWindows team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"


#if wxUSE_DIRDLG

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/log.h"
    #include "wx/sizer.h"
    #include "wx/statbox.h"
    #include "wx/radiobox.h"
    #include "wx/checkbox.h"
    #include "wx/button.h"
    #include "wx/filedlg.h"
#endif

#include "wx/generic/dirctrlg.h"

#include "wx/wupdlock.h"
#include "wx/stdpaths.h"
#include "wx/filename.h"

#include "widgets.h"

#include "icons/dirctrl.xpm"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
    DirCtrlPage_Reset = wxID_HIGHEST,
    DirCtrlPage_SetPath,
    DirCtrlPage_Ctrl
};

static const wxString stdPaths[] =
{
    "&none",
    "&config",
    "&data",
    "&documents",
    "&local data",
    "&plugins",
    "&resources",
    "&user config",
    "&user data",
    "&user local data"
};

enum
{
    stdPathUnknown = 0,
    stdPathConfig,
    stdPathData,
    stdPathDocuments,
    stdPathLocalData,
    stdPathPlugins,
    stdPathResources,
    stdPathUserConfig,
    stdPathUserData,
    stdPathUserLocalData,
    stdPathMax
};

// ----------------------------------------------------------------------------
// CheckBoxWidgetsPage
// ----------------------------------------------------------------------------

class DirCtrlWidgetsPage : public WidgetsPage
{
public:
    DirCtrlWidgetsPage(WidgetsBookCtrl *book, wxVector<wxBitmapBundle>& imaglist);
    virtual ~DirCtrlWidgetsPage() {}

    virtual wxWindow *GetWidget() const override { return m_dirCtrl; }
    virtual void RecreateWidget() override { CreateDirCtrl(); }

    // lazy creation of the content
    virtual void CreateContent() override;

protected:
    // event handlers
    void OnButtonSetPath(wxCommandEvent& event);
    void OnButtonReset(wxCommandEvent& event);
    void OnStdPath(wxCommandEvent& event);
    void OnCheckBox(wxCommandEvent& event);
    void OnRadioBox(wxCommandEvent& event);
    void OnSelChanged(wxTreeEvent& event);
    void OnFileActivated(wxTreeEvent& event);

    // reset the control parameters
    void Reset();

    // (re)create the m_dirCtrl
    void CreateDirCtrl(bool defaultPath = false);

    // the controls
    // ------------

    // the control itself and the sizer it is in
    wxGenericDirCtrl *m_dirCtrl;

    // the text entries for command parameters
    wxTextCtrl *m_path;

    wxRadioBox *m_radioStdPath;

    // flags
    wxCheckBox *m_chkDirOnly,
               *m_chk3D,
               *m_chkFirst,
               *m_chkFilters,
               *m_chkLabels,
               *m_chkMulti;

    // filters
    wxCheckBox *m_fltr[3];

private:
    wxDECLARE_EVENT_TABLE();
    DECLARE_WIDGETS_PAGE(DirCtrlWidgetsPage)
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(DirCtrlWidgetsPage, WidgetsPage)
    EVT_BUTTON(DirCtrlPage_Reset, DirCtrlWidgetsPage::OnButtonReset)
    EVT_BUTTON(DirCtrlPage_SetPath, DirCtrlWidgetsPage::OnButtonSetPath)
    EVT_CHECKBOX(wxID_ANY, DirCtrlWidgetsPage::OnCheckBox)
    EVT_RADIOBOX(wxID_ANY, DirCtrlWidgetsPage::OnRadioBox)
    EVT_DIRCTRL_SELECTIONCHANGED(DirCtrlPage_Ctrl, DirCtrlWidgetsPage::OnSelChanged)
    EVT_DIRCTRL_FILEACTIVATED(DirCtrlPage_Ctrl, DirCtrlWidgetsPage::OnFileActivated)
wxEND_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_WIDGETS_PAGE(DirCtrlWidgetsPage, "DirCtrl",
                       GENERIC_CTRLS
                       );

DirCtrlWidgetsPage::DirCtrlWidgetsPage(WidgetsBookCtrl *book,
                                       wxVector<wxBitmapBundle>& imaglist)
                   :WidgetsPage(book, imaglist, dirctrl_xpm)
{
    m_dirCtrl = nullptr;
}

void DirCtrlWidgetsPage::CreateContent()
{
    wxSizer *sizerTop = new wxBoxSizer(wxHORIZONTAL);

    // left pane
    wxStaticBoxSizer *sizerLeft = new wxStaticBoxSizer(wxVERTICAL, this, "Dir control details");
    wxStaticBox* const sizerLeftBox = sizerLeft->GetStaticBox();

    sizerLeft->Add( CreateSizerWithTextAndButton( DirCtrlPage_SetPath , "Set &path", wxID_ANY, &m_path, sizerLeftBox),
                    0, wxALL | wxALIGN_RIGHT , 5 );

    wxStaticBoxSizer *sizerFlags = new wxStaticBoxSizer(wxVERTICAL, sizerLeftBox, "&Flags");
    wxStaticBox* const sizerFlagsBox = sizerFlags->GetStaticBox();

    m_chkDirOnly = CreateCheckBoxAndAddToSizer(sizerFlags, "wxDIRCTRL_DIR_ONLY", wxID_ANY, sizerFlagsBox);
    m_chk3D      = CreateCheckBoxAndAddToSizer(sizerFlags, "wxDIRCTRL_3D_INTERNAL", wxID_ANY, sizerFlagsBox);
    m_chkFirst   = CreateCheckBoxAndAddToSizer(sizerFlags, "wxDIRCTRL_SELECT_FIRST", wxID_ANY, sizerFlagsBox);
    m_chkFilters = CreateCheckBoxAndAddToSizer(sizerFlags, "wxDIRCTRL_SHOW_FILTERS", wxID_ANY, sizerFlagsBox);
    m_chkLabels  = CreateCheckBoxAndAddToSizer(sizerFlags, "wxDIRCTRL_EDIT_LABELS", wxID_ANY, sizerFlagsBox);
    m_chkMulti   = CreateCheckBoxAndAddToSizer(sizerFlags, "wxDIRCTRL_MULTIPLE", wxID_ANY, sizerFlagsBox);
    sizerLeft->Add(sizerFlags, wxSizerFlags().Expand().Border());

    wxStaticBoxSizer *sizerFilters = new wxStaticBoxSizer(wxVERTICAL, sizerLeftBox, "&Filters");
    wxStaticBox* const sizerFiltersBox = sizerFilters->GetStaticBox();

    m_fltr[0] = CreateCheckBoxAndAddToSizer(sizerFilters, wxString::Format("all files (%s)|%s",
                            wxFileSelectorDefaultWildcardStr, wxFileSelectorDefaultWildcardStr),
                            wxID_ANY, sizerFiltersBox);
    m_fltr[1] = CreateCheckBoxAndAddToSizer(sizerFilters, "C++ files (*.cpp; *.h)|*.cpp;*.h", wxID_ANY, sizerFiltersBox);
    m_fltr[2] = CreateCheckBoxAndAddToSizer(sizerFilters, "PNG images (*.png)|*.png", wxID_ANY, sizerFiltersBox);
    sizerLeft->Add(sizerFilters, wxSizerFlags().Expand().Border());

    wxButton *btn = new wxButton(sizerFiltersBox, DirCtrlPage_Reset, "&Reset");
    sizerLeft->Add(btn, 0, wxALIGN_CENTRE_HORIZONTAL | wxALL, 15);

    // keep consistency between enum and labels of radiobox
    wxCOMPILE_TIME_ASSERT( stdPathMax == WXSIZEOF(stdPaths), EnumForRadioBoxMismatch);

    // middle pane
    m_radioStdPath = new wxRadioBox(this, wxID_ANY, "Standard path",
                                    wxDefaultPosition, wxDefaultSize,
                                    WXSIZEOF(stdPaths), stdPaths, 1);

    // right pane
    m_dirCtrl = new wxGenericDirCtrl(
        this,
        DirCtrlPage_Ctrl,
        wxDirDialogDefaultFolderStr,
        wxDefaultPosition,
        wxDefaultSize,
        0
    );

    // the 3 panes panes compose the window
    sizerTop->Add(sizerLeft, 0, (wxALL & ~wxLEFT), 10);
    sizerTop->Add(m_radioStdPath, 0, wxGROW | wxALL , 10);
    sizerTop->Add(m_dirCtrl, 1, wxGROW | (wxALL & ~wxRIGHT), 10);

    SetSizer(sizerTop);

    // final initializations
    Reset();
}

void DirCtrlWidgetsPage::Reset()
{
    m_path->Clear();

    m_chkDirOnly->SetValue(false);
    m_chk3D->SetValue(false);
    m_chkFirst->SetValue(false);
    m_chkFilters->SetValue(false);
    m_chkLabels->SetValue(false);
    m_chkMulti->SetValue(false);

    m_radioStdPath->SetSelection(0);

    for ( size_t i = 0; i < WXSIZEOF(m_fltr); ++i )
        m_fltr[i]->SetValue(false);

    CreateDirCtrl(true);
}

void DirCtrlWidgetsPage::CreateDirCtrl(bool defaultPath)
{
    wxWindowUpdateLocker noUpdates(this);

    long style = GetAttrs().m_defaultFlags;
    if ( m_chkDirOnly->IsChecked() )
        style |= wxDIRCTRL_DIR_ONLY;
    if ( m_chk3D->IsChecked() )
        style |= wxDIRCTRL_3D_INTERNAL;
    if ( m_chkFirst->IsChecked() )
        style |= wxDIRCTRL_SELECT_FIRST;
    if ( m_chkFilters->IsChecked() )
        style |= wxDIRCTRL_SHOW_FILTERS;
    if ( m_chkLabels->IsChecked() )
        style |= wxDIRCTRL_EDIT_LABELS;
    if ( m_chkMulti->IsChecked() )
        style |= wxDIRCTRL_MULTIPLE;


    wxGenericDirCtrl *dirCtrl = new wxGenericDirCtrl(
        this,
        DirCtrlPage_Ctrl,
        defaultPath ? wxString(wxDirDialogDefaultFolderStr) : m_dirCtrl->GetPath(),
        wxDefaultPosition,
        wxDefaultSize,
        style
    );

    wxString filter;
    for (int i = 0; i < 3; ++i)
    {
        if (m_fltr[i]->IsChecked())
        {
            if (!filter.IsEmpty())
                filter += "|";
            filter += m_fltr[i]->GetLabel();
        }
    }
    dirCtrl->SetFilter(filter);

    // update sizer's child window
    GetSizer()->Replace(m_dirCtrl, dirCtrl, true);

    // update our pointer
    delete m_dirCtrl;
    m_dirCtrl = dirCtrl;

    NotifyWidgetRecreation(m_dirCtrl);

    // relayout the sizer
    GetSizer()->Layout();
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void DirCtrlWidgetsPage::OnButtonSetPath(wxCommandEvent& WXUNUSED(event))
{
    m_dirCtrl->SetPath(m_path->GetValue());
}

void DirCtrlWidgetsPage::OnButtonReset(wxCommandEvent& WXUNUSED(event))
{
    Reset();

    CreateDirCtrl();
}

void DirCtrlWidgetsPage::OnCheckBox(wxCommandEvent& WXUNUSED(event))
{
    CreateDirCtrl();
}

void DirCtrlWidgetsPage::OnRadioBox(wxCommandEvent& WXUNUSED(event))
{
    wxString path;

    wxTheApp->SetAppName("widgets");
    wxStandardPathsBase& stdp = wxStandardPaths::Get();

    switch ( m_radioStdPath->GetSelection() )
    {
        default:
        case stdPathUnknown:
        case stdPathMax:
            // leave path
            break;

        case stdPathConfig:
            path = stdp.GetConfigDir();
            break;

        case stdPathData:
            path = stdp.GetDataDir();
            break;

        case stdPathDocuments:
            path = stdp.GetDocumentsDir();
            break;

        case stdPathLocalData:
            path = stdp.GetLocalDataDir();
            break;

        case stdPathPlugins:
            path = stdp.GetPluginsDir();
            break;

        case stdPathResources:
            path = stdp.GetResourcesDir();
            break;

        case stdPathUserConfig:
            path = stdp.GetUserConfigDir();
            break;

        case stdPathUserData:
            path = stdp.GetUserDataDir();
            break;

        case stdPathUserLocalData:
            path = stdp.GetUserLocalDataDir();
            break;
    }

    m_dirCtrl->SetPath(path);

    // Notice that we must use wxFileName comparison instead of simple wxString
    // comparison as the paths returned may differ by case only.
    if ( wxFileName(m_dirCtrl->GetPath()) != path )
    {
        wxLogMessage("Failed to go to \"%s\", the current path is \"%s\".",
                     path, m_dirCtrl->GetPath());
    }
}

void DirCtrlWidgetsPage::OnSelChanged(wxTreeEvent& event)
{
    if ( m_dirCtrl )
    {
        wxLogMessage("Selection changed to \"%s\"",
                     m_dirCtrl->GetPath(event.GetItem()));
    }

    event.Skip();
}

void DirCtrlWidgetsPage::OnFileActivated(wxTreeEvent& event)
{
    if ( m_dirCtrl )
    {
        wxLogMessage("File activated \"%s\"",
                     m_dirCtrl->GetPath(event.GetItem()));
    }

    event.Skip();
}

#endif // wxUSE_DIRDLG
