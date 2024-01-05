﻿///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/statusbr.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
#include <wx/treectrl.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class Frame
///////////////////////////////////////////////////////////////////////////////
class Frame : public wxFrame
{
	private:

	protected:
		enum
		{
			ID_LOG_SHOW = 1000,
			ID_LOG_LEVEL_ERROR,
			ID_LOG_LEVEL_WARNING,
			ID_LOG_LEVEL_MESSAGE,
			ID_LOG_LEVEL_STATUS,
			ID_LOG_LEVEL_INFO
		};

		wxStatusBar* m_statusBar;
		wxMenuBar* m_menubar;
		wxMenu* m_menu_file;
		wxMenu* m_menu_commands;
		wxMenu* m_menu_log;
		wxAuiToolBar* m_auiToolBar;
		wxAuiToolBarItem* m_toolPort;
		wxAuiToolBarItem* m_toolAttach;
		wxAuiToolBarItem* m_toolDetach;
		wxTreeCtrl* m_treeCtrlList;

		// Virtual event handlers, override them in your derived class
		virtual void on_close( wxCloseEvent& event ) { event.Skip(); }
		virtual void on_exit( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_list( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_attach( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_detach( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_port( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_log_show( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_log_show_update_ui( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void on_log_level( wxCommandEvent& event ) { event.Skip(); }


	public:

		Frame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("USBIP"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		wxAuiManager m_mgr;

		~Frame();

};
