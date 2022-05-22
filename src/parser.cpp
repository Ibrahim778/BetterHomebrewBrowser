#include <stdio.h>
#include <paf.h>
#include <json.h>
#include <libsysmodule.h>

#include "parser.hpp"
#include "csv.h"
#include "common.hpp"
#include "main.hpp"
#include "utils.hpp"

using namespace sce;
using namespace Json;
using namespace parser;

HomebrewList list;

#pragma region JsonClassses
class JsonAllocator : public MemAllocator
{
public:
    JsonAllocator(){};
    ~JsonAllocator(){};
    virtual void *allocate(size_t size, void *user_data)
    {
        void *p = sce_paf_malloc(size);
        return p;
    }

    virtual void deallocate(void *ptr, void *user_data)
    {
        sce_paf_free(ptr);
    }

    virtual void notifyError (int32_t error, size_t size, void *userData )
    {
        switch(error){
        case SCE_JSON_ERROR_NOMEM:
            print("allocate Fail. size = %ld\n", size);
            sceKernelExitProcess(0);
        default:
            print("unknown[%#x]\n", error);
            break;
        }
    }
};

class NullAccess
{
public:
	Json::Value vboolean;
	Json::Value vinteger;
	Json::Value vuinteger;
	Json::Value vreal;
	Json::Value vstring;
	Json::Value vnull;

public:
	NullAccess()
	{
		vboolean.set(false);
		vinteger.set((int64_t)-99);
		vuinteger.set((uint64_t)99);
		vreal.set((double)-0.99);
		vstring.set(Json::String("invalid string"));
		vnull.set(Json::kValueTypeNull);
	}
	~NullAccess(){};
	static const Json::Value& NullAccessCB(Json::ValueType accesstype, const Json::Value* parent, void* context)
	{
		print("CB(%d)[parent:%d]\n", accesstype, parent->getType());

		switch(accesstype){
		case Json::kValueTypeBoolean:
			return reinterpret_cast<NullAccess*>(context)->vboolean;
		case Json::kValueTypeInteger:
			return reinterpret_cast<NullAccess*>(context)->vinteger;
		case Json::kValueTypeUInteger:
			return reinterpret_cast<NullAccess*>(context)->vuinteger;
		case Json::kValueTypeReal:
			return reinterpret_cast<NullAccess*>(context)->vreal;
		case Json::kValueTypeString:
			return reinterpret_cast<NullAccess*>(context)->vstring;
		default:
			break;
		}

		return (reinterpret_cast<NullAccess*>(context))->vnull;
	}

};

#pragma endregion JsonClassses

HomebrewList::HomebrewList()
{
    head = NULL;
    tail = NULL;
    num = 0;
}

void HomebrewList::PrintAll()
{
#ifdef _DEBUG
    node *current = head;
    while (current != NULL)
    {
        print("%s\n", current->info.title.data);
        current = current->next;
    }
#endif
}

void HomebrewList::AddFromPointer(node *p)
{
    node *tmp = new node;
    sce_paf_memset(tmp, 0, sizeof(tmp->info));

    tmp->next = NULL;

    sce_paf_memcpy(tmp, p, sizeof(node));

    if (head == NULL)
    {
        head = tmp;
        tail = tmp;
    }
    else
    {
        tail->next = tmp;
        tail = tail->next;
    }
    num ++;
}

HomebrewList::homeBrewInfo *HomebrewList::AddNode()
{
    node *tmp = new node;
    sce_paf_memset(&tmp->info, 0, sizeof(tmp->info));
    tmp->tex = NULL;
    tmp->next = NULL;

    if (head == NULL)
    {
        head = tmp;
        tail = tmp;
    }
    else
    {
        tail->next = tmp;
        tail = tail->next;
    }
    num ++;
    return &tmp->info;
}

void HomebrewList::Clear(bool deleteTex)
{
    if(head == NULL) return;

    node *temp;
    while(head != NULL)
    {
        temp = head;
        head = head->next;
        if(deleteTex)
            Utils::DeleteTexture(&temp->tex);
        delete temp;
    }

    num = 0;
}

int HomebrewList::GetNumByCategory(int cat)
{
    if(cat == -1) return num;

    int i = 0;
    node *n = list.head;
    while (n != NULL)
    {
        if(n->info.type == cat) i++;
        n = n->next;
    }

    return i;
}

void HomebrewList::CopyTo(HomebrewList *list)
{
    if(list == NULL) return;
    node *n = head;
    for(int i = 0; i < num && n != NULL, n = n->next; i++)
    {
        list->AddFromPointer(n);
    }
}

HomebrewList::node *HomebrewList::GetByIndex(int n)
{
    node *node = head;

    for (int i = 0; i < n && node != NULL; i++)
        node = node->next;

    return node;
}

HomebrewList::node *HomebrewList::GetByCategoryIndex(int n, int category)
{
    node *node = head;

    for (int i = 0; i < n && node != NULL; i++)
    {
        if(node->info.type != category && category != -1)
        {
            i--;
        }
        node = node->next;
    }
    return node;
}

HomebrewList::homeBrewInfo *HomebrewList::Get(const char *id)
{
    node *curr = head;
    while (curr != NULL)
    {
        if(sce_paf_strcmp(curr->info.id.data, id))
            return &curr->info;
        
        curr = curr->next;
    }
    return NULL;
}

HomebrewList::node *HomebrewList::Find(const char *name)
{
    node *curr = head;
    int nLen = sce_paf_strlen(name);
    while(curr != NULL)
    {
        if(sce_paf_strncmp(curr->info.title.data, name, nLen) == 0)
            return curr;
        
        curr = curr->next;
    }
    return NULL;
}

//Credit to CreepNT for this
void HomebrewList::RemoveNode(const char *tag)
{
    //Check head isn't NULL
    if (head == NULL)
        return;

    //First, handle the case where we free the head
    if (sce_paf_strcmp(head->info.id.data, tag) == 0)
    {
        node *nodeToDelete = head;
        head = head->next;
        sce_paf_free(nodeToDelete);
        return;
    }

    //Bail out if the head is the only node
    if (head->next == NULL)
        return;

    //Else, try to locate node we're asked to remove
    node **pCurrentNodeNext = &head; //This points to the current node's `next` field (or to pHead)
    while (1)
    {
        if (sce_paf_strcmp((*pCurrentNodeNext)->info.id.data, tag) == 0) //pCurrentNodeNext points to the pointer that points to the node we need to delete
            break;

        //If the next node's next is NULL, we reached the end of the list. Bail out.
        if ((*pCurrentNodeNext)->next == NULL)
            return;

        pCurrentNodeNext = &(*pCurrentNodeNext)->next;
    }
    node *nodeToDelete = *pCurrentNodeNext;
    *pCurrentNodeNext = (*pCurrentNodeNext)->next;
    Utils::DeleteTexture(&nodeToDelete->tex);
    sce_paf_free(nodeToDelete);
    num --;
}

#define SET_STRING(pafstring, jsonstring) { if(rootval[i][jsonstring] != NULL) { pafstring = rootval[i][jsonstring].getString().c_str(); } } 
 
/*
void parser::ParseJson(const char *path)
{
    list.Clear(true);
    JsonAllocator allocator;
    InitParameter initParam((MemAllocator *)&allocator, 0, 1024);

    Initializer initializer;
    initializer.initialize(&initParam);

    Value rootval;
    NullAccess na;
    rootval.setNullAccessCallBack(NullAccess::NullAccessCB, &na);
    int r = Parser::parse(rootval, path);
    if(r < 0)
    {
        print("Parser::parse() -> 0x%X\n", r);
        return;
    }
    SceInt32 ItemNum = rootval.count();
    for(int i = 0; i < ItemNum; i++)
    {
        if(rootval[i] == NULL) return;
        HomebrewList::homeBrewInfo *info = list.AddNode();

        SET_STRING(info->titleID, "titleid");
        SET_STRING(info->id, "id");
        SET_STRING(info->icon0, "icon");
        
        if(rootval[i]["icon"] != NULL)
            info->icon0Local.Setf(VITADB_ICON_SAVE_PATH "/%s", rootval[i]["icon"].getString().c_str());

        SET_STRING(info->title, "name");
        info->title.ToWString(&info->wstrtitle);

        SET_STRING(info->download_url, "url");
        SET_STRING(info->credits, "author");
        SET_STRING(info->options, "data");
        SET_STRING(info->description, "long_description");
        SET_STRING(info->screenshot_url, "screenshots");
        SET_STRING(info->version, "version");        

        if(rootval[i]["type"] != NULL)
            info->type = (HomebrewList::Category)sce_paf_strtoul(rootval[i]["type"].getString().c_str(), NULL, 10);

        SET_STRING(info->size, "size");
    }
}*/