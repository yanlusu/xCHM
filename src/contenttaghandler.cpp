/*

  Copyright (C) 2003  Razvan Cojocaru <razvanco@gmx.net>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include <chmlistctrl.h>
#include <contenttaghandler.h>
#include <wx/wx.h>
#include <wx/fontmap.h>


ContentTagHandler::ContentTagHandler(wxFontEncoding enc, bool useEnc,
				     wxTreeCtrl* tree, CHMListCtrl *list)
	: _level(0), _treeCtrl(tree), _listCtrl(list), _enc(enc), 
	  _useEnc(useEnc)
{
	if(!_treeCtrl && !_listCtrl)
		return;

	if(_treeCtrl)
		_parents[_level] = _treeCtrl->AddRoot(_("Topics"));
}


bool ContentTagHandler::HandleTag(const wxHtmlTag& tag)
{
	if(!_treeCtrl && !_listCtrl)
		return FALSE;

	if (tag.GetName() == wxT("UL"))
	{
		if(_treeCtrl && _level >= TREE_BUF_SIZE)
			return FALSE;
		
		++_level;
		ParseInner(tag);
		--_level;

		return TRUE;

	} else if(!tag.GetName().CmpNoCase(wxT("OBJECT"))) {
        
		/* 
		   Valid HHW's file may contain only two object tags: 
 
		   <OBJECT type="text/site properties"> 
		   <param name="ImageType" value="Folder"> 
		   </OBJECT> 
 
		   or 
 
		   <OBJECT type="text/sitemap"> 
		   <param name="Name" value="main page"> 
		   <param name="Local" value="another.htm"> 
		   </OBJECT> 
 
		   We're interested in the latter.
		*/
		
		if(!tag.GetParam(wxT("TYPE")).CmpNoCase(wxT("text/sitemap"))) {

			// handle broken <UL> tags that would
			// otherwise SEGFAULT by trying to access
			// _parents[-1].
			int parentIndex = _level ? _level - 1 : 0;

			_title = _url = wxEmptyString;
			ParseInner(tag);

			if(_title.IsEmpty())
				_title = wxT("?");

			if(_treeCtrl && !_url.IsEmpty()) {

				_parents[_level] = 
					_treeCtrl->AppendItem(
						_parents[parentIndex],
						_title, -1, -1,
						new URLTreeItem(_url));
				if(!_level)
					_parents[0] = _treeCtrl->GetRootItem();
			}

			return TRUE;
		}
		
		return FALSE;
	} else { // "PARAM"

		if(!tag.GetParam(wxT("NAME")).CmpNoCase(wxT("Name"))) {

			if(_title.IsEmpty()) {
				if(_useEnc) {
					wxCSConv cv(wxFontMapper::Get()->
						    GetEncodingName(_enc));

					const wxString s = 
						tag.GetParam(wxT("VALUE"));
					const size_t len = s.length();
					wxCharBuffer buf(len);
					char *p = buf.data();
				
					for ( size_t n = 0; n < len; n++ ) {
						wxASSERT_MSG(s[n] < 0x100, 
							     wxT("non ASCII"
								 " char?") );
						*p++ = s[n];
					}

					_title = wxString(buf, cv);
				} else 
					_title = tag.GetParam(wxT("VALUE"));
			}
		}

		if(!tag.GetParam(wxT("NAME")).CmpNoCase(wxT("Local"))) {
			_url = tag.GetParam(wxT("VALUE"));
		}

		if(_listCtrl && !_url.IsEmpty() && !_title.IsEmpty()) {
			_listCtrl->AddPairItem(_title, _url);
			_title = _url = wxEmptyString;
		}

		return FALSE;
	}
}


