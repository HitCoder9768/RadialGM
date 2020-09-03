#include "RoomEditor.h"
#include "Components/ArtManager.h"
#include "Components/QMenuView.h"
#include "MainWindow.h"
#include "ui_RoomEditor.h"

#include "Models/ImmediateMapper.h"
#include "Models/MessageModel.h"
#include "Models/RepeatedMessageModel.h"
#include "Models/TreeSortFilterProxyModel.h"
#include "Room.pb.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMouseEvent>

#include <algorithm>

using View = buffers::resources::Room::View;
using Layer = buffers::resources::Room::Layer;

RoomEditor::RoomEditor(MessageModel* model, QWidget* parent) : BaseEditor(model, parent), _ui(new Ui::RoomEditor) {
  _ui->setupUi(this);
  connect(_ui->actionSave, &QAction::triggered, this, &BaseEditor::OnSave);

  _ui->roomPreviewBackground->SetAssetView(_ui->roomView);

  _nodeMapper->addMapping(_ui->roomName, TreeNode::kNameFieldNumber);
  _nodeMapper->toFirst();

  _resMapper->addMapping(_ui->speedSpinBox, Room::kSpeedFieldNumber);
  _resMapper->addMapping(_ui->widthSpinBox, Room::kWidthFieldNumber);
  _resMapper->addMapping(_ui->heightSpinBox, Room::kHeightFieldNumber);
  _resMapper->addMapping(_ui->clearCheckBox, Room::kClearDisplayBufferFieldNumber);
  _resMapper->addMapping(_ui->persistentCheckBox, Room::kPersistentFieldNumber);
  _resMapper->addMapping(_ui->captionLineEdit, Room::kCaptionFieldNumber);

  _resMapper->addMapping(_ui->enableViewsCheckBox, Room::kEnableViewsFieldNumber);
  _resMapper->addMapping(_ui->clearViewportCheckBox, Room::kClearViewBackgroundFieldNumber);
  _resMapper->toFirst();

  _viewMapper = new ImmediateDataWidgetMapper(this);
  _viewMapper->addMapping(_ui->viewVisibleCheckBox, View::kVisibleFieldNumber);

  _viewMapper->addMapping(_ui->cameraXSpinBox, View::kXviewFieldNumber);
  _viewMapper->addMapping(_ui->cameraYSpinBox, View::kYviewFieldNumber);
  _viewMapper->addMapping(_ui->cameraWidthSpinBox, View::kWviewFieldNumber);
  _viewMapper->addMapping(_ui->cameraHeightSpinBox, View::kHviewFieldNumber);

  _viewMapper->addMapping(_ui->viewportXSpinBox, View::kXportFieldNumber);
  _viewMapper->addMapping(_ui->viewportYSpinBox, View::kYportFieldNumber);
  _viewMapper->addMapping(_ui->viewportWidthSpinBox, View::kWportFieldNumber);
  _viewMapper->addMapping(_ui->viewportHeightSpinBox, View::kHportFieldNumber);

  _viewMapper->addMapping(_ui->followingHBorderSpinBox, View::kHborderFieldNumber);
  _viewMapper->addMapping(_ui->followingVBorderSpinBox, View::kVborderFieldNumber);
  _viewMapper->addMapping(_ui->followingHSpeedSpinBox, View::kHspeedFieldNumber);
  _viewMapper->addMapping(_ui->followingVSpeedSpinBox, View::kVspeedFieldNumber);
  _viewMapper->toFirst();

  connect(_ui->addLayerButton, &QAbstractButton::clicked, [=]() {
    auto layerModel = _ui->layersListView->model();
    int row = layerModel->rowCount();
    layerModel->insertRow(row);
    layerModel->setData(layerModel->index(row,Room::Layer::kNameFieldNumber),"Layer");
    layerModel->setData(layerModel->index(row,Room::Layer::kDepthFieldNumber),0);
    layerModel->setData(layerModel->index(row,Room::Layer::kVisibleFieldNumber),true);
  });
  connect(_ui->deleteLayerButton, &QAbstractButton::clicked, [=]() {
    auto layerModel = _ui->layersListView->model();
    layerModel->removeRow(_ui->layersListView->currentIndex().row());
  });

  QMenuView* objMenu = new QMenuView(this);
  TreeSortFilterProxyModel* treeProxy = new TreeSortFilterProxyModel(this);
  treeProxy->SetFilterType(TreeNode::TypeCase::kObject);
  treeProxy->setSourceModel(MainWindow::treeModel.get());
  objMenu->setModel(treeProxy);
  _ui->objectSelectButton->setMenu(objMenu);
  _ui->objectSelectButton->setPopupMode(QToolButton::MenuButtonPopup);

  auto objects = treeProxy
      ->match(treeProxy->index(0, 0), TreeModel::UserRoles::TypeCaseRole,
              TypeCase::kObject, 1, Qt::MatchRecursive);
  if (!objects.empty()) {
    QModelIndex firstObjIdx = objects.first();
    QString firstObj = firstObjIdx.data(Qt::DisplayRole).toString();
    _ui->objectSelectButton->setIcon(firstObjIdx.data(Qt::DecorationRole).value<QIcon>());
    _ui->currentObject->setText(firstObj);
  }

  connect(objMenu, &QMenuView::triggered, [=](const QModelIndex &index) {
    _ui->currentObject->setText(treeProxy->data(index, Qt::DisplayRole).toString());
    _ui->objectSelectButton->setIcon(treeProxy->data(index, Qt::DecorationRole).value<QIcon>());
  });

  connect(_ui->currentViewComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int index) { _viewMapper->setCurrentIndex(index); });

  cursorPositionLabel = new QLabel();
  _ui->statusBar->addWidget(cursorPositionLabel);

  _assetNameLabel = new QLabel("obj_xxx");
  _ui->statusBar->addWidget(_assetNameLabel);

  // This updates all the model views in the event of a sprite is changed
  connect(MainWindow::resourceMap.get(), &ResourceModelMap::DataChanged, this, [this]() {
    _ui->elementsListView->reset();
    _ui->propertiesView->reset();
  });

  RebindSubModels();
}

RoomEditor::~RoomEditor() { delete _ui; }

void RoomEditor::RebindSubModels() {
  _roomModel = _model->GetSubModel<MessageModel*>(TreeNode::kRoomFieldNumber);
  _ui->roomView->SetResourceModel(_roomModel);

  connect(_ui->roomPreviewBackground, &AssetScrollAreaBackground::MouseMoved, this, &RoomEditor::MouseMoved);
  connect(_ui->roomPreviewBackground, &AssetScrollAreaBackground::MousePressed, this, &RoomEditor::MousePressed);
  connect(_ui->roomPreviewBackground, &AssetScrollAreaBackground::MouseReleased, this, &RoomEditor::MouseReleased);

  RepeatedMessageModel* lm = _roomModel->GetSubModel<RepeatedMessageModel*>(Room::kLayersFieldNumber);
  lm->setHeaderData(Room::Layer::kNameFieldNumber, Qt::Horizontal,
                    tr("Name"), Qt::DisplayRole);
  lm->setHeaderData(Room::Layer::kDepthFieldNumber, Qt::Horizontal,
                    tr("Depth"), Qt::DisplayRole);
  lm->setHeaderData(Room::Layer::kVisibleFieldNumber, Qt::Horizontal,
                    tr(""), Qt::DisplayRole);
  lm->setHeaderData(Room::Layer::kVisibleFieldNumber, Qt::Horizontal,
                    tr("Visible"), Qt::ToolTipRole);
  lm->setHeaderData(Room::Layer::kVisibleFieldNumber, Qt::Horizontal,
                    QIcon(":/actions/find.png"), Qt::DecorationRole);
  _ui->layersListView->setModel(lm);
  for (int c = 0; c < lm->columnCount(); ++c) {
    if (c != Room::Layer::kNameFieldNumber &&
        c != Room::Layer::kDepthFieldNumber &&
        c != Room::Layer::kVisibleFieldNumber)
      _ui->layersListView->hideColumn(c);
  }

  QSortFilterProxyModel* emp = new QSortFilterProxyModel(this);
  _ui->elementsListView->setModel(emp);

  connect(_ui->layersListView->selectionModel(), &QItemSelectionModel::selectionChanged,
          [=](const QItemSelection& selected, const QItemSelection& /*deselected*/) {
            if (selected.empty()) {
              _ui->propertiesView->setModel(nullptr);
              _ui->propertiesView->setEnabled(false);
              emp->setSourceModel(nullptr);
              _ui->elementsListWidget->setEnabled(false);
              return;
            }
            auto selectedIndex = selected.indexes().first();
            auto currentLayerModel = lm->GetSubModel<MessageModel*>(selectedIndex.row());
            _ui->propertiesView->setModel(currentLayerModel);
            _ui->propertiesView->setEnabled(true);

            RepeatedMessageModel* im = currentLayerModel->GetSubModel<RepeatedMessageModel*>(
                  Layer::kInstancesFieldNumber);
            emp->setSourceModel(im);
            _ui->elementsListWidget->setEnabled(true);

            for (int c = 0; c < im->columnCount(); ++c) {
              if (c != Room::Instance::kNameFieldNumber && c != Room::Instance::kObjectTypeFieldNumber &&
                  c != Room::Instance::kIdFieldNumber)
                _ui->elementsListView->hideColumn(c);
              else
                _ui->elementsListView->resizeColumnToContents(c);
            }

            _ui->elementsListView->header()->swapSections(Room::Instance::kNameFieldNumber,
                                                           Room::Instance::kObjectTypeFieldNumber);
          });

  RepeatedMessageModel* vm = _roomModel->GetSubModel<RepeatedMessageModel*>(Room::kViewsFieldNumber);
  _viewMapper->setModel(vm);

  connect(_ui->elementsListView->selectionModel(), &QItemSelectionModel::selectionChanged,
          [=](const QItemSelection& selected, const QItemSelection& /*deselected*/) {
            if (selected.empty()) return;
            auto currentLayerModel = lm->GetSubModel<MessageModel*>(_ui->layersListView->currentIndex().row());
            RepeatedMessageModel* im = currentLayerModel->GetSubModel<RepeatedMessageModel*>(
                  Layer::kInstancesFieldNumber);
            auto selectedIndex = selected.indexes().first();
            auto currentInstanceModel = im->GetSubModel<MessageModel*>(selectedIndex.row());
            _ui->propertiesView->setModel(currentInstanceModel);
          });

  BaseEditor::RebindSubModels();
}

void RoomEditor::MouseMoved(int x, int y) {
  const GridDimensions g = _ui->roomView->GetGrid();
  QPoint mousePos(x, y);
  cursorPositionLabel->setText(tr("X %0, Y %1").arg(mousePos.x()).arg(mousePos.y()));
}

void RoomEditor::MousePressed(Qt::MouseButton button) {
  auto elementsProxy = static_cast<QSortFilterProxyModel*>(_ui->elementsListView->model());
  auto layerElements = static_cast<RepeatedMessageModel*>(elementsProxy->sourceModel());
  if (layerElements == nullptr) return;
  if (button == Qt::MouseButton::LeftButton) {
    auto index = layerElements->rowCount();
    layerElements->insertRow(index);
    layerElements->SetData(_ui->currentObject->text(), index, Room::Instance::kObjectTypeFieldNumber);
  }
}

void RoomEditor::MouseReleased(Qt::MouseButton button) {

}

void RoomEditor::on_actionZoomIn_triggered() { _ui->roomPreviewBackground->ZoomIn(); }

void RoomEditor::on_actionZoomOut_triggered() { _ui->roomPreviewBackground->ZoomOut(); }

void RoomEditor::on_actionZoom_triggered() { _ui->roomPreviewBackground->ResetZoom(); }
