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

unsigned int sceLibcHeapSize = 6 * 1024 * 1024;

linked_list list;
extern Page *currPage;
extern graphics::Texture *BrokenTex;
extern Plugin *mainPlugin;

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

linked_list::linked_list()
{
    head = NULL;
    tail = NULL;
    num = 0;
}

void linked_list::printall()
{
#ifdef _DEBUG
    node *current = head;
    while (current != NULL)
    {
        print("%s, ", current->widget.title.data);
        current = current->next;
    }
#endif
}

homeBrewInfo *linked_list::add_node()
{
    node *tmp = new node;
    sce_paf_memset(&tmp->widget, 0, sizeof(tmp->widget));
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
    return &tmp->widget;
}

void linked_list::clear()
{
    if(head == NULL) return;

    node *temp;
    while(head != NULL)
    {
        temp = head;
        head = head->next;
        delete temp;
    }

    num = 0;
}

node *linked_list::getByNum(int n)
{
    node *node = head;

    for (int i = 0; i < n && node != NULL; i++)
        node = node->next;

    return node;
}

homeBrewInfo *linked_list::get(const char *id)
{
    node *curr = head;
    while (curr != NULL)
    {
        if(sce_paf_strcmp(curr->widget.id.data, id))
            return &curr->widget;
        
        curr = curr->next;
    }
    return NULL;
}

//Credit to CreepNT for this
void linked_list::remove_node(const char *tag)
{
    //Check head isn't NULL
    if (head == NULL)
        return;

    //First, handle the case where we free the head
    if (sce_paf_strcmp(head->widget.id.data, tag) == 0)
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
        if (sce_paf_strcmp((*pCurrentNodeNext)->widget.id.data, tag) == 0) //pCurrentNodeNext points to the pointer that points to the node we need to delete
            break;

        //If the next node's next is NULL, we reached the end of the list. Bail out.
        if ((*pCurrentNodeNext)->next == NULL)
            return;

        pCurrentNodeNext = &(*pCurrentNodeNext)->next;
    }
    node *nodeToDelete = *pCurrentNodeNext;
    *pCurrentNodeNext = (*pCurrentNodeNext)->next;
    sce_paf_free(nodeToDelete);

    num --;
}

void parseJson(const char *path)
{
    list.clear();
    JsonAllocator allocator;
    InitParameter initParam((MemAllocator *)&allocator, 0, 512);

    Initializer initializer;
    initializer.initialize(&initParam);

    Value rootval;
    NullAccess na;
    rootval.setNullAccessCallBack(NullAccess::NullAccessCB, &na);
    Parser::parse(rootval, path);

    SceInt32 ItemNum = rootval.count();
    for(int i = 0; i < ItemNum; i++)
    {
        homeBrewInfo *info = list.add_node();

        info->titleID.Set(rootval[i]["titleid"].getString().c_str());
        info->id.Set(rootval[i]["id"].getString().c_str());
        info->icon0.Set(rootval[i]["icon"].getString().c_str());

        info->icon0Local.Setf(VITADB_ICON_SAVE_PATH "/%s", rootval[i]["icon"].getString().c_str());

        info->title.Set(rootval[i]["name"].getString().c_str());
        info->download_url.Set(rootval[i]["url"].getString().c_str());
        info->credits.Set(rootval[i]["author"].getString().c_str());
        info->options.Set(rootval[i]["data"].getString().c_str());
        info->description.Set(rootval[i]["long_description"].getString().c_str());
        info->screenshot_url.Set(rootval[i]["screenshots"].getString().c_str());
    }
}

void parseCSV(const char *path)
{
    list.clear();

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
                homeBrewInfo *info = list.add_node();

                info->id.Set(parsed[0]);
                info->title.Set(parsed[1]);
                info->credits.Set(parsed[2]);
                info->icon0.Set(parsed[3]);
                info->download_url.Set(parsed[5]);
                info->options.Set(parsed[13]);
                info->description.Set(parsed[0]);
                
                //In CBPS DB titleID is used for id, so if any contradict _n is added, this is done to get just the id
                char titleID[10] = {0};
                sceClibStrncat(titleID, parsed[0], 10);

                info->titleID.Set(titleID);

                info->icon0Local.Setf(VITADB_ICON_SAVE_PATH "/%s.png",  info->id.data);

            }
        }

        if(line != NULL)
            free_csv_line(parsed);

    }

	sce_paf_fclose(file);
}
 