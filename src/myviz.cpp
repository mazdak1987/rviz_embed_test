#include "myviz/myviz.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QSlider>

#include "rclcpp/clock.hpp"
#include "rviz_common/render_panel.hpp"
#include "rviz_common/ros_integration/ros_node_abstraction.hpp"
#include "rviz_common/visualization_manager.hpp"
#include "rviz_rendering/render_window.hpp"

MyViz::MyViz(
  QApplication * app,
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr rviz_ros_node,
  QWidget * parent)
: app_(app), rviz_ros_node_(rviz_ros_node), QMainWindow(parent)
{
  // Construct the layout
  QLabel * thickness_label = new QLabel("Line Thickness");
  QSlider * thickness_slider = new QSlider(Qt::Horizontal);
  thickness_slider->setMinimum(1);
  thickness_slider->setMaximum(100);
  QLabel * cell_size_label = new QLabel("Cell Size");
  QSlider * cell_size_slider = new QSlider(Qt::Horizontal);
  cell_size_slider->setMinimum(1);
  cell_size_slider->setMaximum(100);
  QGridLayout * controls_layout = new QGridLayout();
  controls_layout->addWidget(thickness_label, 0, 0);
  controls_layout->addWidget(thickness_slider, 0, 1);
  controls_layout->addWidget(cell_size_label, 1, 0);
  controls_layout->addWidget(cell_size_slider, 1, 1);

  // Add visualization
  main_layout = new QVBoxLayout;
  main_layout->addLayout(controls_layout);
  central_widget = new QWidget();
  main_layout->setSpacing(0);
  main_layout->setMargin(0);

  // Initialize the classes we need from rviz
  initializeRViz();

  central_widget->setLayout(main_layout);
  setCentralWidget(central_widget);
  main_layout->addWidget(render_panel_);

  // Signals
  connect(thickness_slider, SIGNAL(valueChanged(int)), this, SLOT(setThickness(int)));
  connect(cell_size_slider, SIGNAL(valueChanged(int)), this, SLOT(setCellSize(int)));

  // Display the rviz grid plugin
  DisplayGrid();

  // Intialize the sliders
  thickness_slider->setValue(25);
  cell_size_slider->setValue(10);
}

QWidget *
MyViz::getParentWindow()
{
  return this;
}

rviz_common::PanelDockWidget *
MyViz::addPane(const QString & name, QWidget * pane, Qt::DockWidgetArea area, bool floating)
{
  // TODO(mjeronimo)
  return nullptr;
}

void
MyViz::setStatus(const QString & message)
{
  // TODO(mjeronimo)
}

void MyViz::DisplayGrid()
{
  grid_ = manager_->createDisplay("rviz_default_plugins/Grid", "adjustable grid", true);
  assert(grid_ != NULL);
  grid_->subProp("Line Style")->setValue("Billboards");
  grid_->subProp("Color")->setValue(QColor(Qt::yellow));
}

void MyViz::initializeRViz()
{
  app_->processEvents();
  render_panel_ = new rviz_common::RenderPanel(central_widget);
  app_->processEvents();
  render_panel_->getRenderWindow()->initialize();
  auto clock = rviz_ros_node_.lock()->get_raw_node()->get_clock();
  manager_ = new rviz_common::VisualizationManager(render_panel_, rviz_ros_node_, this, clock);
  render_panel_->initialize(manager_);
  app_->processEvents();
  
  
  //trying to add text overlay on render_panel_
  QRect screenGeo = QRect(20, 20, 200,200);//QGuiApplication::primaryScreen()->geometry()
  auto hudTexture = Ogre::TextureManager::getSingleton().createManual(
        "HUDTexture",        // name
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Ogre::TEX_TYPE_2D,   // type
        screenGeo.width(), screenGeo.height(),
        0,                   // number of mipmaps
        Ogre::PF_A8R8G8B8,   // pixel format chosen to match a format Qt can use
        Ogre::TU_DEFAULT    //TU_DEFAULT usage
        );
  Ogre::MaterialPtr HudMaterial = Ogre::MaterialManager::getSingleton().create("HUDMaterial", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
  HudMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("HUDTexture");
  HudMaterial->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_MODULATE);
  Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
  auto overlay = overlayManager.create("OverlayName");
  Ogre::PanelOverlayElement* overlayElement = static_cast<Ogre::PanelOverlayElement*>(overlayManager.createOverlayElement("Panel", "PanelName"));// Create a panel
  overlayElement->setMetricsMode(Ogre::GMM_PIXELS);
  overlayElement->setPosition(0, 0);
  overlayElement->setDimensions(screenGeo.width(), screenGeo.height());
  overlayElement->setMaterialName("HUDMaterial");
  overlay->add2D(overlayElement);//Add the panel to the overlay

  Ogre::HardwarePixelBufferSharedPtr pixelBuffer = hudTexture->getBuffer();// Get the pixel buffer
  pixelBuffer->lock( Ogre::HardwareBuffer::HBL_DISCARD ); //Lock the pixel buffer (for best performance use HBL_DISCARD!) // HBL_NORMAL
  const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock(); //get a pixel box
  Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);
  memset(pDest, 0, hudTexture->getWidth() * hudTexture->getHeight() * 4);// (uint8 * 4 = uint32) the buffer content is the colors R,G,B,A. Filling with zeros gets a 100% transparent image
  QImage Hud(pDest, hudTexture->getWidth(), hudTexture->getHeight(), QImage::Format_ARGB32); // tell QImage to use OUR buffer and a compatible image buffer format
  QPainter painter(&Hud);
  QRect borderRect = QRect(10, 10, 200, 200);
  QFont font;
  font.setPointSize(40);
  painter.setFont(font);
  painter.setPen(Qt::red);
  painter.drawText(borderRect, Qt::AlignLeft, "my_text_to_show");
  //
  
  
  
  manager_->initialize();
  manager_->startUpdate();
}

void MyViz::setThickness(int thickness_percent)
{
  if (grid_ != NULL) {
    grid_->subProp("Line Style")->subProp("Line Width")->setValue(thickness_percent / 100.0f);
  }
}

void MyViz::setCellSize(int cell_size_percent)
{
  if (grid_ != NULL) {
    grid_->subProp("Cell Size")->setValue(cell_size_percent / 10.0f);
  }
}

void MyViz::closeEvent(QCloseEvent * event)
{
  QMainWindow::closeEvent(event);
  rclcpp::shutdown();
}
