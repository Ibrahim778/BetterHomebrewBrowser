#include "parser.hpp"
#include "main.hpp"
#include "pagemgr.hpp"
#include <stdio.h>
#include <paf.h>
#include "csv.h"
#include "common.hpp"
#include <json.h>
#include "utils.hpp"

using namespace sce;
using namespace Json;

LinkedList list;

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

LinkedList::LinkedList()
{
    head = NULL;
    tail = NULL;
    num = 0;
}

void LinkedList::PrintAll()
{
#ifdef _DEBUG
    node *current = head;
    while (current != NULL)
    {
        print("%s, ", current->info.title.data);
        current = current->next;
    }
#endif
}

void LinkedList::AddFromPointer(node *p)
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

homeBrewInfo *LinkedList::AddNode()
{
    node *tmp = new node;
    sce_paf_memset(&tmp->info, 0, sizeof(tmp->info));
    tmp->tex = new graphics::Texture();
    tmp->tex->texSurface = NULL;
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

void LinkedList::Clear(bool deleteTex)
{
    if(head == NULL) return;

    node *temp;
    while(head != NULL)
    {
        temp = head;
        head = head->next;
        if(deleteTex)
            BHBB::Utils::DeleteTexture(temp->tex);
        delete temp;
    }

    num = 0;
}

int LinkedList::GetNumByCategory(int cat)
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

void LinkedList::CopyTo(LinkedList *list)
{
    if(list == NULL) return;
    node *n = head;
    for(int i = 0; i < num && n != NULL, n = n->next; i++)
    {
        list->AddFromPointer(n);
    }
}

node *LinkedList::GetByIndex(int n)
{
    node *node = head;

    for (int i = 0; i < n && node != NULL; i++)
        node = node->next;

    return node;
}

node *LinkedList::GetByCategoryIndex(int n, int category)
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

homeBrewInfo *LinkedList::Get(const char *id)
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

node *LinkedList::Find(const char *name)
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
void LinkedList::RemoveNode(const char *tag)
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
    BHBB::Utils::DeleteTexture(nodeToDelete->tex);
    sce_paf_free(nodeToDelete);
    num --;
}

#define SET_STRING(pafString, jsonString) { if(rootval[i][jsonString] != NULL) { pafString.Set(rootval[i][jsonString].getString().c_str()); } } 
 
void parseJson(const char *path)
{
    list.Clear(true);
    JsonAllocator allocator;
    InitParameter initParam((MemAllocator *)&allocator, 0, 512);

    Initializer initializer;
    initializer.initialize(&initParam);

    Value rootval;
    NullAccess na;
    rootval.setNullAccessCallBack(NullAccess::NullAccessCB, &na);
    int r = Parser::parse(rootval, path);
    if(r < 0) return;

    SceInt32 ItemNum = rootval.count();
    for(int i = 0; i < ItemNum; i++)
    {
        if(rootval[i] == NULL) return;
        homeBrewInfo *info = list.AddNode();

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
            info->type = (Category)sce_paf_strtoul(rootval[i]["type"].getString().c_str(), NULL, 10);

        SET_STRING(info->size, "size");
    }
}

void parseCSV(const char *path)
{
    list.Clear(true);

    FILE *file = sce_paf_fopen(path, "rb");
    if(file == NULL) return;

    int done = 0;
    int err;
    while (done != 1)
    {
        char *line = fread_csv_line(file, 1024, &done, &err);
        char **parsed = parse_csv(line);

        if(parsed != NULL)
        {
            int dead = 0;
            for (int i = 0; i < 17 && !dead; i++)
            {
                dead = parsed[i] == NULL;
            }
            
            if(!dead && sce_paf_strncmp(parsed[14], "VPK", 3) == 0)
            {
                homeBrewInfo *info = list.AddNode();

                info->id.Set(parsed[0]);

                info->title.Set(parsed[1]);

                info->title.ToWString(&info->wstrtitle);           
                info->credits.Set(parsed[2]);
                info->icon0.Set(parsed[3]);
                info->download_url.Set(parsed[5]);
                info->options.Set(parsed[13]);
                info->description.Set(parsed[0]);

                //In CBPS DB titleID is used for id, so if any contradict _n is added, this is done to get just the id
                char titleID[10] = {0};
                sceClibStrncat(titleID, parsed[0], 10);

                info->titleID.Set(titleID);
                info->icon0Local.Setf(CBPSDB_ICON_SAVE_PATH "/%s.png",  info->id.data);
                info->version.SetEmpty();
            }
        }

        if(line != NULL)
            free_csv_line(parsed);

    }

	sce_paf_fclose(file);
}
 