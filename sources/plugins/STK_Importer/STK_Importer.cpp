//**************************************************************************/
// Copyright (c) 1998-2018 Autodesk, Inc.
// All rights reserved.
// 
// Use of this software is subject to the terms of the Autodesk license 
// agreement provided at the time of installation or download, or which 
// otherwise accompanies this software in either electronic or hard copy form.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/
#define NOMINMAX

#include "STK_Importer.h"
#include "xr_file_system.h"
#include "xr_log.h"
#include "xr_object.h"
#include "xr_dm.h"
#include "xr_ogf.h"
#include "xr_ogf_v4.h"
#include "xr_skl_motion.h"
#include "xr_object.h"
#include "xr_sdk_version.h"
#include "MeshNormalSpec.h"
#include <atlbase.h>
#include <atlconv.h>
#include <map>
#include <algorithm>
#include <set>

using namespace xray_re;
using SmoothingGR = std::map<uint32_t, std::vector<uint32_t>>;

#define STK_Importer_CLASS_ID Class_ID(0x49576ca1, 0x3f5aacb2)
class STK_Importer : public SceneImport
{
public:
	//Constructor/Destructor
	STK_Importer();
	virtual ~STK_Importer();

	virtual int				ExtCount();					// Number of extensions supported
	virtual const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	virtual const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	virtual const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	virtual const TCHAR *	AuthorName();				// ASCII Author name
	virtual const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	virtual const TCHAR *	OtherMessage1();			// Other message #1
	virtual const TCHAR *	OtherMessage2();			// Other message #2
	virtual unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	virtual void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	virtual int				DoImport(const TCHAR *name,ImpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE);	// Import file

	void					Init(HWND hWnd);
	int                     ObjectImportShoc(HWND hWnd, ImpInterface* importerInt, Interface* ip);
	int                     ObjectImportCop(HWND hWnd, ImpInterface* importerInt, Interface* ip);
	void                    import_mesh(const xr_mesh* mesh, const xr_bone_vec& bones, ImpInterface* importerInt, Interface* ip, int mode = 0);
	bool                    DoImportSHOC(TCHAR* name);

public:
	bool                    bSHOC;
	void                    SaveParams(HWND hWnd);

	int                     ObjectImport(HWND hWnd, ImpInterface* importerInt, Interface* ip);
				  TCHAR *	FileName;

};



class STK_ImporterClassDesc : public ClassDesc2 
{
public:
	virtual int           IsPublic() override                       { return TRUE; }
	virtual void*         Create(BOOL /*loading = FALSE*/) override { return new STK_Importer(); }
	virtual const TCHAR * ClassName() override                      { return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID     SuperClassID() override                   { return SCENE_IMPORT_CLASS_ID; }
	virtual Class_ID      ClassID() override                        { return STK_Importer_CLASS_ID; }
	virtual const TCHAR*  Category() override                       { return GetString(IDS_CATEGORY); }

	virtual const TCHAR*  InternalName() override                   { return _T("STK_Importer"); } // Returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE     HInstance() override                      { return hInstance; } // Returns owning module handle


};

ClassDesc2* GetSTK_ImporterDesc()
{
	static STK_ImporterClassDesc STK_ImporterDesc;
	return &STK_ImporterDesc; 
}

template <typename T>
std::vector<T> intersection(std::vector<T>& v1,
	std::vector<T>& v2) {
	std::vector<T> v3;

	std::sort(v1.begin(), v1.end());
	std::sort(v2.begin(), v2.end());

	std::set_intersection(v1.begin(), v1.end(),
		v2.begin(), v2.end(),
		back_inserter(v3));
	return v3;
}

void STK_Importer::SaveParams(HWND hWnd)
{
	bSHOC = IsDlgButtonChecked(hWnd, IDC_RADIO1);
}

void STK_Importer::import_mesh(const xr_mesh* mesh, const xr_bone_vec& bones, ImpInterface* importerInt, Interface* ip, int mode)
{
	TriObject* model = CreateNewTriObject();

	Mesh* mod = &model->GetMesh();

	Mesh* ne = CreateNewMesh();

	const std::vector<fvector3>& points = mesh->points();
	const lw_face_vec& faces = mesh->faces();
	const std::vector<uint32_t>& sgroups = mesh->sgroups();
	const std::vector<fvector3>& normals = mesh->vnorm();
	const lw_vmref_vec& vmrefs = mesh->vmrefs();
	const xr_vmap_vec& vmaps = mesh->vmaps();

	SmoothingGR SG_, SGFaces[32]; // Смух группы общие
	//std::sort(SG[0].begin(), SG[0].end(), cmp);

	mod->setNumVerts(points.size());
	mod->setNumFaces(faces.size());
	mod->setMapSupport(1);
	mod->maps[1].setNumVerts(vmrefs.size());
	mod->maps[1].setNumFaces(faces.size());

	int vert_id = 0;
	for (std::vector<fvector3>::const_iterator it = points.begin(), end = points.end();
		it != end; ++it) {
		mod->setVert(vert_id++, it->xyz);
	}

	int face_id = 0;
	for (lw_face_vec_cit it = faces.begin(), end = faces.end(); it != end; ++it) 
	{	
		mod->faces[face_id].setVerts(it->v[0], it->v[1], it->v[2]);
		mod->faces[face_id].setEdgeVisFlags(1, 1, 1);
		mod->maps[1].tf[face_id].setTVerts(it->ref[0], it->ref[1], it->ref[2]);
		face_id++;
	}
	
	for (int i = 0; i < vmrefs.size(); i++)
	{
		size_t j = vmrefs[i][0].vmap;
		size_t k = vmrefs[i][0].offset;

		const xr_uv_vmap* vmap = static_cast<const xr_uv_vmap*>(vmaps[j]);

		mod->maps[1].tv[i].Set(vmap->uvs()[k].x, 1 - vmap->uvs()[k].y, 0);
	}

	int surf_id = 0;
	for (auto& it : mesh->surfmaps())
	{
		const xr_surfmap* smap = it;

		for (int id = 0; id < smap->faces.size(); id++)
		{
			mod->faces[smap->faces[id]].setMatID(surf_id);
		}

		surf_id++;
	}
	if (mode == 0 && !sgroups.empty())
	{
		for (int i = 0; i < faces.size(); i++)
		{
			if (sgroups.size() > i)
			{
				DWORD sm = sgroups[i];
				sm += 3;
				SG_[sm].push_back(i);
			}
		}
		//std::vector<std::vector<uint32_t>> sgroups_faces;

		// Сортировка по количеству фейсов в группе
		/*std::vector<std::pair<int, int>> size_key;
		for (auto& x : SG_)
			size_key.emplace_back(x.second.size(), x.first);

		std::sort(size_key.begin(), size_key.end());

		std::reverse(size_key.begin(), size_key.end());

		//for (auto& x : size_key)
			//sgroups_faces.push_back(SG_[x.second]);

		int idx = 0;

		// Распихиваем группы в порядке возрастания по порядку, с нуля
		for (auto& x : size_key)
		{
			SGFaces[0][idx].assign(SG_[x.second].begin(), SG_[x.second].end());
			idx++;
		}
		*/
		int idx = 0;
		for (auto& x : SG_)
		{
			SGFaces[0][idx].assign(x.second.begin(), x.second.end());
			idx++;
		}

		for (int i = 0; i < 32; i++)
		{
			if (SGFaces[i].size() > 1 && !SGFaces[i].empty())
			{
				std::vector<uint32_t> var;
				for (auto& face : SGFaces[i][0])
				{
					#pragma message(TODO("Позже переписать всю эту дичь и распихать по функциям"))
					const uint32_t* verts = faces[face].v;//mod->faces[face].getAllVerts();
					var.push_back(verts[0]); var.push_back(verts[1]); var.push_back(verts[2]);
				}
				
				for (int j = 1; j < SGFaces[i].size(); j++)
				{
					std::vector<uint32_t> v;
					for (auto& face : SGFaces[i][j])
					{
						const uint32_t* verts = faces[face].v;//mod->faces[face].getAllVerts();
						v.push_back(verts[0]); v.push_back(verts[1]); v.push_back(verts[2]);
					}

					std::vector<uint32_t> result = intersection(v, var);

					if (result.empty())
					{
						var.insert(var.end(), v.begin(), v.end());
						SGFaces[i][0].insert(SGFaces[i][0].end(), SGFaces[i][j].begin(), SGFaces[i][j].end());
					}
					else
					{
						int k = SGFaces[i + 1].size();
						std::vector<uint32_t>& SGR = SGFaces[i + 1][k];
						SGR.insert(SGR.end(),SGFaces[i][j].begin(), SGFaces[i][j].end());
					}

				}
			}
		}

		mod->checkNormals(TRUE);
		mod->SpecifyNormals();

		for (uint32_t i = 0; i < 32; i++)
		{
			if (!SGFaces[i].empty())
			{
				for (auto& face : SGFaces[i][0])
					mod->faces[face].setSmGroup((2^i));

			}

		}

	}
	if (mode == 2 && !normals.empty())
	{
		mod->checkNormals(TRUE);
		mod->SpecifyNormals();
		MeshNormalSpec* specNorms = mod->GetSpecifiedNormals();

		if (nullptr != specNorms)
		{
			specNorms->ClearAndFree();
			specNorms->SetNumNormals(normals.size());
			specNorms->SetNumFaces(faces.size());
			Point3* norms = specNorms->GetNormalArray();

			for (int i = 0; i < normals.size() / 3; i++) {
				fvector3 v0 = normals[i * 3];
				fvector3 v1 = normals[i * 3 + 2];
				fvector3 v2 = normals[i * 3 + 1];
				norms[i * 3] = Point3(v0.x, v0.y, v0.z);
				norms[i * 3 + 1] = Point3(v1.x, v1.y, v1.z);
				norms[i * 3 + 2] = Point3(v2.x, v2.y, v2.z);
			}

			MeshNormalFace* pFaces = specNorms->GetFaceArray();
			for (int i = 0; i < faces.size(); i++) {
				const lw_face& tri = faces[i];
				pFaces[i].SpecifyNormalID(0, i * 3);
				pFaces[i].SpecifyNormalID(1, i * 3 + 2);
				pFaces[i].SpecifyNormalID(2, i * 3 + 1);
			}




			specNorms->SetAllExplicit(true);
			specNorms->CheckNormals();
		}
	}

	mod->InvalidateTopologyCache();
	mod->InvalidateGeomCache();
	mod->InvalidateEdgeList();
	mod->buildBoundingBox();
	//mod->buildNormals();

	ImpNode* node = importerInt->CreateNode();
	if (!node) {
		delete model;
	}
	Matrix3 matIdentity;
	matIdentity.IdentityMatrix();
	node->Reference(model);
	//node->SetTransform(0, matIdentity);

	USES_CONVERSION;

	importerInt->AddNodeToScene(node);
	node->SetName(A2T(mesh->name().c_str()));
	
}

INT_PTR CALLBACK STK_ImporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	//static STK_Importer* imp = (STK_Importer*)lParam;
	static STK_Importer* imp = NULL;

	switch(message) 
	{
		case WM_INITDIALOG:
			//imp = (STK_Importer *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			imp = (STK_Importer*)lParam;
			imp->Init(hWnd);
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) 
			{
				case IDC_IMPORT:
					imp->SaveParams(hWnd);
					EndDialog(hWnd, 1);
					return TRUE;
			}
	}
	return FALSE;
}


//--- STK_Importer -------------------------------------------------------
STK_Importer::STK_Importer()
{
	bSHOC = false;
}

STK_Importer::~STK_Importer() 
{

}

int STK_Importer::ObjectImport(HWND hWnd, ImpInterface* importerInt, Interface* ip)
{
	if (bSHOC)
		return ObjectImportShoc(hWnd,importerInt,ip);
	else
		return ObjectImportCop(hWnd,importerInt,ip);
}

int STK_Importer::ObjectImportShoc(HWND hWnd, ImpInterface* importerInt, Interface* ip)
{
	xr_object* object = new xr_object;

	USES_CONVERSION;

	if (object->load_object(T2A(FileName)))
	{
		const xr_bone_vec& bones = object->bones();

		for (auto& it : object->meshes())
		{
			import_mesh(it, bones, importerInt, ip);
		}
		return IMPEXP_SUCCESS;
	}
	return IMPEXP_FAIL;
}
int STK_Importer::ObjectImportCop(HWND hWnd, ImpInterface* importerInt, Interface* ip)
{
	xr_object* object = new xr_object;

	USES_CONVERSION;

	if (object->load_object(T2A(FileName)))
	{
		const xr_bone_vec& bones = object->bones();

		for (auto& it : object->meshes())
		{
			import_mesh(it, bones, importerInt, ip, 2);
		}
		return IMPEXP_SUCCESS;
	}
	return IMPEXP_FAIL;

}

void STK_Importer::Init(HWND hWnd)
{
	CheckDlgButton(hWnd, IDC_RADIO1, 1);
	bSHOC = false;
}

int STK_Importer::ExtCount()
{
	#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *STK_Importer::Ext(int /*n*/)
{		
	#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("object");
}

const TCHAR *STK_Importer::LongDesc()
{
	#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	return _T("STALKER SDK model file");
}
	
const TCHAR *STK_Importer::ShortDesc() 
{			
	#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("STALKER");
}

const TCHAR *STK_Importer::AuthorName()
{			
	#pragma message(TODO("Return ASCII Author name"))
	return _T("Mortan");
}

const TCHAR *STK_Importer::CopyrightMessage() 
{	
	#pragma message(TODO("Return ASCII Copyright message"))
	return _T("Forward Frontier 2021");
}

const TCHAR *STK_Importer::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *STK_Importer::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int STK_Importer::Version()
{				
	#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void STK_Importer::ShowAbout(HWND /*hWnd*/)
{			
	// Optional
}

DWORD WINAPI fn(LPVOID arg) {
	return(0);
}

int STK_Importer::DoImport(const TCHAR* filename, ImpInterface* importerInt, Interface* ip, BOOL suppressPrompts)
{
	#pragma message(TODO("Implement the actual file import here and"))	

	/*if(!suppressPrompts)
	{
		FileName = (TCHAR*)filename;

		return DialogBoxParam(hInstance,
			MAKEINTRESOURCE(IDD_PANEL),
			GetActiveWindow(),
			STK_ImporterOptionsDlgProc, (LPARAM)this);		
	}*/
	
	if(!DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_PANEL),GetActiveWindow(),STK_ImporterOptionsDlgProc, (LPARAM)this) )
		return IMPEXP_CANCEL;

	ip->ProgressStart(L"Importing...", TRUE, fn, NULL);

	FileName = (TCHAR*)filename;

	//ObjectImport(GetActiveWindow());

	//ip->FileReset(TRUE);
	//importerInt->NewScene();

	ObjectImport(GetActiveWindow(), importerInt,  ip);

	importerInt->RedrawViews();
	ip->ProgressEnd();

	return IMPEXP_SUCCESS;

	//#pragma message(TODO("return TRUE If the file is imported properly"))
	//return FALSE;
}
	
