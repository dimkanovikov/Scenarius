#include "LocationsNavigator.h"

#include "LocationsNavigatorItemDelegate.h"

#include <QLabel>
#include <QToolButton>
#include <QListView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>

using UserInterface::LocationsNavigator;
using UserInterface::LocationsNavigatorItemDelegate;


LocationsNavigator::LocationsNavigator(QWidget *parent) :
	QWidget(parent),
	m_title(new QLabel(this)),
	m_addLocation(new QToolButton(this)),
	m_editLocation(new QToolButton(this)),
	m_removeLocation(new QToolButton(this)),
	m_refreshLocations(new QToolButton(this)),
	m_endTitle(new QLabel(this)),
	m_navigator(new QListView(this)),
	m_navigatorProxyModel(new QSortFilterProxyModel(m_navigator))
{
	initView();
	initConnections();
	initStyleSheet();
}

void LocationsNavigator::setModel(QAbstractItemModel* _model)
{
	m_navigatorProxyModel->setSourceModel(_model);
	m_navigator->setModel(m_navigatorProxyModel);

	//
	// Настраиваем запрос на изменение при изменении текущего элемента
	//
	connect(m_navigator->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(aboutEditLocation()));
}

void LocationsNavigator::selectFirstLocation()
{
	selectLocation(m_navigatorProxyModel->index(0, 0));
}

void LocationsNavigator::selectLocation(const QString& _name)
{
	const QModelIndex matchStartFrom = m_navigatorProxyModel->index(0, 0);
	QModelIndexList matches = m_navigatorProxyModel->match(matchStartFrom, Qt::DisplayRole, _name);
	if (matches.size() > 0) {
		selectLocation(matches.first());
	}
}

void LocationsNavigator::selectLocation(const QModelIndex& _index)
{
	if (m_navigator->model() != 0
		&& m_navigator->model()->rowCount() > 0) {
		//
		// Выделим первую локацию
		//
		m_navigator->setCurrentIndex(_index);

		//
		// Готовим к изменению
		//
		aboutEditLocation();
	}
}

void LocationsNavigator::aboutEditLocation()
{
	emit editLocation(selectedLocationName());
}

void LocationsNavigator::aboutRemoveLocation()
{
	emit removeLocation(selectedLocationName());
}

QString LocationsNavigator::selectedLocationName() const
{
	QModelIndex currentSelected = m_navigator->selectionModel()->currentIndex();
	QString userName = m_navigator->model()->data(currentSelected).toString();
	return userName;
}

void LocationsNavigator::initView()
{
	m_title->setText(tr("Locations"));
	m_title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_endTitle->setFixedWidth(1);

	m_addLocation->setIcon(QIcon(":/Graphics/Icons/Editing/add.png"));
	m_editLocation->setIcon(QIcon(":/Graphics/Icons/Editing/edit.png"));
	m_removeLocation->setIcon(QIcon(":/Graphics/Icons/Editing/delete.png"));
	m_refreshLocations->setIcon(QIcon(":/Graphics/Icons/Editing/refresh.png"));

	m_navigatorProxyModel->sort(0);

	m_navigator->setAlternatingRowColors(true);
	m_navigator->setItemDelegate(new LocationsNavigatorItemDelegate(m_navigator));
	m_navigator->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	QHBoxLayout* topLayout = new QHBoxLayout;
	topLayout->setContentsMargins(QMargins());
	topLayout->setSpacing(0);
	topLayout->addWidget(m_title);
	topLayout->addWidget(m_addLocation);
	topLayout->addWidget(m_editLocation);
	topLayout->addWidget(m_removeLocation);
	topLayout->addWidget(m_refreshLocations);
	topLayout->addWidget(m_endTitle);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(QMargins());
	layout->setSpacing(0);
	layout->addLayout(topLayout);
	layout->addWidget(m_navigator);

	setLayout(layout);
}

void LocationsNavigator::initConnections()
{
	connect(m_addLocation, SIGNAL(clicked()), this, SIGNAL(addLocation()));
	connect(m_editLocation, SIGNAL(clicked()), this, SLOT(aboutEditLocation()));
	connect(m_removeLocation, SIGNAL(clicked()), this, SLOT(aboutRemoveLocation()));
	connect(m_refreshLocations, SIGNAL(clicked()), this, SIGNAL(refreshLocations()));
}

void LocationsNavigator::initStyleSheet()
{
	m_title->setProperty("inTopPanel", true);
	m_title->setProperty("topPanelTopBordered", true);
	m_endTitle->setProperty("inTopPanel", true);
	m_endTitle->setProperty("topPanelTopBordered", true);
	m_endTitle->setProperty("topPanelRightBordered", true);

	m_addLocation->setProperty("inTopPanel", true);
	m_editLocation->setProperty("inTopPanel", true);
	m_removeLocation->setProperty("inTopPanel", true);
	m_refreshLocations->setProperty("inTopPanel", true);

	m_navigator->setProperty("mainContainer", true);
}
