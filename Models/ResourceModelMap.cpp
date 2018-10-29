#include "Models/ResourceModelMap.h"
#include "MainWindow.h"

ResourceModelMap::ResourceModelMap(buffers::TreeNode *root, QObject* parent) : QObject(parent) {
  recursiveBindRes(root, this);
}

void ResourceModelMap::recursiveBindRes(buffers::TreeNode *node, QObject* parent) {

    for (int i=0; i < node->child_size(); ++i) {
      buffers::TreeNode* child = node->mutable_child(i);
      if (child->folder()) {
        recursiveBindRes(child, parent);
        continue;
      }

      _resources[child->type_case()][QString::fromStdString(child->name())] = new ProtoModel(child, parent);
    }
}

ProtoModel* ResourceModelMap::GetResourceByName(int type, const QString& name) {
  if (_resources[type].contains(name))
    return _resources[type][name];
  else return nullptr;
}

ProtoModel* ResourceModelMap::GetResourceByName(int type, const std::string& name) {
  return GetResourceByName(type, QString::fromStdString(name));
}

void ResourceModelMap::ResourceRenamed(TypeCase type, const QString& oldName, const QString& newName) {
  _resources[type][newName] = _resources[type][oldName];
  _resources[type][oldName] = nullptr;
}

const ProtoModel* GetObjectSprite(const std::string& objName) {
  return GetObjectSprite(QString::fromStdString(objName));
}

const ProtoModel* GetObjectSprite(const QString& objName) {
  ProtoModel* obj = MainWindow::resourceMap->GetResourceByName(TreeNode::kObject, objName);
  if (!obj) return nullptr;
  obj = obj->GetSubModel(TreeNode::kObjectFieldNumber);
  if (!obj) return nullptr;
  const QString spriteName = obj->data(Object::kSpriteNameFieldNumber).toString();
  ProtoModel* spr = MainWindow::resourceMap->GetResourceByName(TreeNode::kSprite, spriteName);
  if (spr) return spr->GetSubModel(TreeNode::kSpriteFieldNumber);
  return nullptr;
}
