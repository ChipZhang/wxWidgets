///////////////////////////////////////////////////////////////////////////////
// Name:        src/gtk/infobar.cpp
// Purpose:     wxInfoBar implementation for GTK
// Author:      Vadim Zeitlin
// Created:     2009-09-27
// Copyright:   (c) 2009 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"


#include "wx/infobar.h"

#if wxUSE_INFOBAR && defined(wxHAS_NATIVE_INFOBAR)

#ifndef WX_PRECOMP
#endif // WX_PRECOMP

#include "wx/vector.h"
#include "wx/stockitem.h"
#include "wx/tooltip.h"

#include "wx/gtk/private.h"
#include "wx/gtk/private/messagetype.h"
#include "wx/gtk/private/mnemonics.h"

// ----------------------------------------------------------------------------
// local classes
// ----------------------------------------------------------------------------

class wxInfoBarGTKImpl
{
public:
    wxInfoBarGTKImpl()
    {
        m_label = nullptr;
        m_close = nullptr;
    }

    // label for the text shown in the bar
    GtkWidget *m_label;

    // the default close button, nullptr if not needed (m_buttons is not empty) or
    // not created yet
    GtkWidget *m_close;

    // information about the buttons added using AddButton()
    struct Button
    {
        Button(GtkWidget *button_, int id_)
            : button(button_),
              id(id_)
        {
        }

        GtkWidget *button;
        int id;
    };
    typedef wxVector<Button> Buttons;

    Buttons m_buttons;
};

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------

extern "C"
{

static void wxgtk_infobar_response(GtkInfoBar * WXUNUSED(infobar),
                                   gint btnid,
                                   wxInfoBar *win)
{
    win->GTKResponse(btnid);
}

static void wxgtk_infobar_close(GtkInfoBar * WXUNUSED(infobar),
                                wxInfoBar *win)
{
    win->GTKResponse(wxID_CANCEL);
}

} // extern "C" section with GTK+ callbacks

// ============================================================================
// wxInfoBar implementation
// ============================================================================

bool wxInfoBar::UseNative() const
{
    // Native GtkInfoBar widget is only available in GTK+ 2.18 and later.
    // Also, if the checkbox is included, then we need to use
    // the generic version.
    return wx_is_at_least_gtk2(18) && !HasFlag(wxINFOBAR_CHECKBOX);
}

bool wxInfoBar::Create(wxWindow *parent, wxWindowID winid, long style)
{
    SetWindowStyle(style);
    if ( !UseNative() )
        return wxInfoBarGeneric::Create(parent, winid, style);

    m_impl = new wxInfoBarGTKImpl;

    // this control is created initially hidden
    Hide();
    if ( !CreateBase(parent, winid) )
        return false;

    // create the info bar widget itself
    m_widget = gtk_info_bar_new();
    wxCHECK_MSG( m_widget, false, "failed to create GtkInfoBar" );
    g_object_ref(m_widget);

    // also create a label which will be used to show our message
    m_impl->m_label = gtk_label_new("");
    gtk_widget_show(m_impl->m_label);

    GtkWidget * const
        contentArea = gtk_info_bar_get_content_area(GTK_INFO_BAR(m_widget));
    wxCHECK_MSG( contentArea, false, "failed to get GtkInfoBar content area" );
    gtk_container_add(GTK_CONTAINER(contentArea), m_impl->m_label);

    // finish creation and connect to all the signals we're interested in
    m_parent->DoAddChild(this);

    PostCreation(wxDefaultSize);

    GTKConnectWidget("response", G_CALLBACK(wxgtk_infobar_response));
    GTKConnectWidget("close", G_CALLBACK(wxgtk_infobar_close));

    // Work around GTK+ bug https://bugzilla.gnome.org/show_bug.cgi?id=710888
    // by disabling the transition when showing it: without this, it's not
    // shown at all.
    //
    // Compile-time check is needed because GtkRevealer is new in 3.10.
#if GTK_CHECK_VERSION(3, 10, 0)
    // Run-time check is needed because the bug was introduced in 3.10 and
    // fixed in 3.22.29 (see 6b4d95e86dabfcdaa805fbf068a99e19736a39a4 and a
    // couple of previous commits in GTK+ repository).
    if ( gtk_check_version(3, 10, 0) == nullptr &&
            gtk_check_version(3, 22, 29) != nullptr )
    {
        GObject* const
            revealer = gtk_widget_get_template_child(GTK_WIDGET(m_widget),
                                                     GTK_TYPE_INFO_BAR,
                                                     "revealer");
        if ( revealer )
        {
            gtk_revealer_set_transition_type(GTK_REVEALER (revealer),
                                             GTK_REVEALER_TRANSITION_TYPE_NONE);
            gtk_revealer_set_transition_duration(GTK_REVEALER (revealer), 0);
        }
    }
#endif // GTK+ >= 3.10

    return true;
}

wxInfoBar::~wxInfoBar()
{
    delete m_impl;
}

void wxInfoBar::ShowMessage(const wxString& msg, int flags)
{
    if ( !UseNative() )
    {
        wxInfoBarGeneric::ShowMessage(msg, flags);
        return;
    }

    // if we don't have any buttons, create a standard close one to give the
    // user at least some way to close the bar
    if ( m_impl->m_buttons.empty() && !m_impl->m_close )
    {
        m_impl->m_close = GTKAddButton(wxID_CLOSE);
    }

    GtkMessageType type;
    if ( wxGTKImpl::ConvertMessageTypeFromWX(flags, &type) )
        gtk_info_bar_set_message_type(GTK_INFO_BAR(m_widget), type);
    gtk_label_set_text(GTK_LABEL(m_impl->m_label), msg.utf8_str());
    gtk_label_set_ellipsize(GTK_LABEL(m_impl->m_label), PANGO_ELLIPSIZE_MIDDLE);

#if wxUSE_TOOLTIPS
    wxToolTip::GTKApply(m_impl->m_label, msg.utf8_str());
#endif

    if ( !IsShown() )
        Show();

    UpdateParent();
}

void wxInfoBar::Dismiss()
{
    if ( !UseNative() )
    {
        wxInfoBarGeneric::Dismiss();
        return;
    }

    Hide();

    UpdateParent();
}

void wxInfoBar::GTKResponse(int btnid)
{
    wxCommandEvent event(wxEVT_BUTTON, btnid);
    event.SetEventObject(this);

    if ( !HandleWindowEvent(event) )
        Dismiss();
}

GtkWidget *wxInfoBar::GTKAddButton(wxWindowID btnid, const wxString& label)
{
    // as GTK+ lays out the buttons vertically, adding another button changes
    // our best size (at least in vertical direction)
    InvalidateBestSize();

    GtkWidget* button = gtk_info_bar_add_button(GTK_INFO_BAR(m_widget),
#ifdef __WXGTK4__
        (label.empty() ? wxConvertMnemonicsToGTK(wxGetStockLabel(btnid)) : label).utf8_str(),
#else
        label.empty() ? wxGetStockGtkID(btnid) : static_cast<const char*>(label.utf8_str()),
#endif
        btnid);

    wxASSERT_MSG( button, "unexpectedly failed to add button to info bar" );

    return button;
}

size_t wxInfoBar::GetButtonCount() const
{
    if ( !UseNative() )
        return wxInfoBarGeneric::GetButtonCount();

    return m_impl->m_buttons.size();
}

wxWindowID wxInfoBar::GetButtonId(size_t idx) const
{
    if ( !UseNative() )
        return wxInfoBarGeneric::GetButtonId(idx);

    wxCHECK_MSG( idx < m_impl->m_buttons.size(), wxID_NONE,
                 "Invalid infobar button position" );

    return m_impl->m_buttons[idx].id;
}

void wxInfoBar::AddButton(wxWindowID btnid, const wxString& label)
{
    if ( !UseNative() )
    {
        wxInfoBarGeneric::AddButton(btnid, label);
        return;
    }

    // if we had created the default close button before, remove it now that we
    // have some user-defined button
    if ( m_impl->m_close )
    {
        gtk_widget_destroy(m_impl->m_close);
        m_impl->m_close = nullptr;
    }

    GtkWidget * const button = GTKAddButton(btnid, label);
    if ( button )
        m_impl->m_buttons.push_back(wxInfoBarGTKImpl::Button(button, btnid));
}

bool wxInfoBar::HasButtonId(wxWindowID btnid) const
{
    if ( !UseNative() )
        return wxInfoBarGeneric::HasButtonId(btnid);

    // as in the generic version, look for the button starting from the end
    const wxInfoBarGTKImpl::Buttons& buttons = m_impl->m_buttons;
    for ( wxInfoBarGTKImpl::Buttons::const_reverse_iterator i = buttons.rbegin();
          i != buttons.rend();
          ++i )
    {
        if ( i->id == btnid )
            return true;
    }

    return false;
}

void wxInfoBar::RemoveButton(wxWindowID btnid)
{
    if ( !UseNative() )
    {
        wxInfoBarGeneric::RemoveButton(btnid);
        return;
    }

    // as in the generic version, look for the button starting from the end
    wxInfoBarGTKImpl::Buttons& buttons = m_impl->m_buttons;
    for ( wxInfoBarGTKImpl::Buttons::reverse_iterator i = buttons.rbegin();
          i != buttons.rend();
          ++i )
    {
        if (i->id == btnid)
        {
            gtk_widget_destroy(i->button);
            buttons.erase(i.base() - 1);

            // see comment in GTKAddButton()
            InvalidateBestSize();

            return;
        }
    }

    wxFAIL_MSG( wxString::Format("button with id %d not found", btnid) );
}

void wxInfoBar::DoApplyWidgetStyle(GtkRcStyle *style)
{
    wxInfoBarGeneric::DoApplyWidgetStyle(style);

    if ( UseNative() )
        GTKApplyStyle(m_impl->m_label, style);
}

#endif // wxUSE_INFOBAR
