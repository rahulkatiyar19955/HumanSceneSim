/*
 *    Copyright (C) 2020 by YOUR NAME HERE
 *
 *    This file is part of RoboComp
 *
 *    RoboComp is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    RoboComp is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RoboComp.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "specificworker.h"
#include <QFileDialog>
#include <vector>

/**
* \brief Default constructor
*/
SpecificWorker::SpecificWorker(MapPrx &mprx) : GenericWorker(mprx)
{
	active = false;
	worldModel = AGMModel::SPtr(new AGMModel());
	worldModel->name = "worldModel";
}

/**
* \brief Default destructor
*/
SpecificWorker::~SpecificWorker()
{
	std::cout << "Destroying SpecificWorker" << std::endl;
}

bool SpecificWorker::setParams(RoboCompCommonBehavior::ParameterList params)
{
	//	THE FOLLOWING IS JUST AN EXAMPLE
	//	To use innerModelPath parameter you should uncomment specificmonitor.cpp readConfig method content
	//	try
	//	{
	//		RoboCompCommonBehavior::Parameter par = params.at("InnerModelPath");
	//		std::string innermodel_path = par.value;
	//		innerModel = std::make_shared(innermodel_path);
	//	}
	//	catch(const std::exception &e) { qFatal("Error reading config params"); }

	innerModel = std::make_shared<InnerModel>(new InnerModel());
	try
	{
		RoboCompAGMWorldModel::World w = agmexecutive_proxy->getModel();
		AGMExecutiveTopic_structuralChange(w);
	}
	catch (...)
	{
		printf("The executive is probably not running, waiting for first AGM model publication...");
	}

	return true;
}

void SpecificWorker::initialize(int period)
{
	std::cout << "Initialize worker" << std::endl;

	try
	{
		RoboCompAGMWorldModel::World w = agmexecutive_proxy->getModel();
		AGMExecutiveTopic_structuralChange(w);
	}
	catch (...)
	{
		printf("The executive is probably not running, waiting for first AGM model publication...");
	}

	this->Period = period;
	timer.start(Period);
	number_of_person = 0;
	initializeUI();
}

void SpecificWorker::initializeUI()
{
	std::cout << " UI initializing" << std::endl;
	connect(browseButton, SIGNAL(pressed()), this, SLOT(browseButtonClicked()));
	connect(browseButton_2, SIGNAL(pressed()), this, SLOT(browseButton2Clicked()));
	connect(play_button, SIGNAL(pressed()), this, SLOT(playButton()));
	connect(nf_button, SIGNAL(pressed()), this, SLOT(nextFrameButton()));
	connect(&playTimer, SIGNAL(timeout()), this, SLOT(play_timer()));
}

void SpecificWorker::play_timer()
{
	// printf("play timepout\n");
	nextFrameButton();
}
// adding multiple persons simultaneously
void SpecificWorker::browseButtonClicked()
{
	std::cout << "Button Clicked..." << std::endl;
	QString folderName = QFileDialog::getExistingDirectory(this, "Open Image", "/home",
														   QFileDialog::ShowDirsOnly |
															   QFileDialog::DontResolveSymlinks);
	folderLoc->setText(folderName);
	QDir dir(folderName);
	dir.setSorting(QDir::Name);
	dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

	QFileInfoList list = dir.entryInfoList();
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);
		string fileN = folderName.toStdString() + "/" + fileInfo.fileName().toStdString();
		myFiles.push_back(make_shared<ifstream>(fileN));
	}
	int per = myFiles.size();
	tableWidget->setRowCount(per);
	personCount->setNum(per);
	// std::cout << total_files << std::endl;

	// tableWidget->setRowCount(total_files);
	// tableWidget->setColumnCount(3);
	// int number_of_person = 0;
	for (auto person_iter : myFiles)
	{
		int genId = initPersons(number_of_person);
		if (genId != -1)
		{
			personGenId.push_back(genId);
			number_of_person++;
		}
		else
		{
			printf("unable to create persons\n");
			break;
		}
	}
}

// adding individual person's CSV
void SpecificWorker::browseButton2Clicked()
{
	QString filename = QFileDialog::getOpenFileName(this, "Open CSV", "/home");
	// std::cout << filename.toStdString() << std::endl;
	myFiles.push_back(make_shared<ifstream>(filename.toStdString()));
	tableWidget->setRowCount(myFiles.size());
	int genId = initPersons(number_of_person);
	if (genId != -1)
	{
		personGenId.push_back(genId);
		number_of_person++;
	}
	else
	{
		printf("unable to create persons\n");
	}
	// extractCSV(filename);
	// printf("over\n");
}
void SpecificWorker::nextFrameButton()
{
	printf("next frame clicked\n");
	int rcount = 0;
	for (auto personData : myFiles)
	{
		vector<double> personPoseData;
		personPoseData = getNextValue(personData);
		string s = std::to_string(personPoseData[0]);
		tableWidget->setItem(rcount, 0, new QTableWidgetItem(tr("%1").arg(personPoseData[0])));
		tableWidget->setItem(rcount, 1, new QTableWidgetItem(tr("%1").arg(personPoseData[1])));
		tableWidget->setItem(rcount, 2, new QTableWidgetItem(tr("%1").arg(personPoseData[2])));
		tableWidget->setItem(rcount, 3, new QTableWidgetItem(tr("%1").arg(personPoseData[3])));
		// for (auto a : personPoseData)
		// {
		// 	std::cout << a << " | ";
		// }
		// std::cout << std::endl;
		movePersons(rcount, personPoseData);
		rcount++;
	}
}
void SpecificWorker::movePersons(int person_ID, vector<double> personPoseData)
{
	RoboCompInnerModelManager::Pose3D pose;
	// multiply by 100 for metre to mm convertion
	pose.x = personPoseData[1] * 1000;
	pose.y = 0;
	pose.z = personPoseData[2] * 1000;
	pose.rx = 0;
	pose.ry = personPoseData[3];
	pose.rz = 0;
	//store new position
	// person->pose = pose;
	//move in RCIS

	try
	{
		innermodelmanager_proxy->setPoseFromParent("person" + std::to_string(person_ID), pose);
	}
	catch (std::exception &e)
	{
		std::cout << "Exception moving person in RCIS: " << e.what() << std::endl;
		return;
	}
	//move in AGM
	AGMModelSymbol::SPtr personParent = worldModel->getParentByLink(personGenId[person_ID], "RT");
	AGMModelEdge &edgeRT = worldModel->getEdgeByIdentifiers(personParent->identifier, personGenId[person_ID], "RT");
	edgeRT.attributes["tx"] = std::to_string(pose.x);
	edgeRT.attributes["ty"] = "0";
	edgeRT.attributes["tz"] = std::to_string(pose.z);
	edgeRT.attributes["rx"] = "0";
	edgeRT.attributes["ry"] = std::to_string(pose.ry);
	edgeRT.attributes["rz"] = "0";
	try
	{
		AGMMisc::publishEdgeUpdate(edgeRT, agmexecutive_proxy);
	}
	catch (std::exception &e)
	{
		std::cout << "Exception moving in AGM: " << e.what() << std::endl;
	}
}
void SpecificWorker::extractCSV(QString path)
{
	// std::cout << "path: " << std::endl;
	// std::cout << path.toStdString() << std::endl;
	// // QString temp_path = folderLoc->text();
	// // std::string path = temp_path.toStdString();
	// int per;
	// try
	// {
	// 	per = std::stoi(personCount->text().toStdString());
	// 	std::cout << per << std::endl;
	// }
	// catch (...)
	// {
	// 	printf("unable to retrieve person count\n");
	// }

	// // openning first file
	// std::string filename = "01.csv";

	try
	{
		std::ifstream tempFile(path.toStdString());
		myFiles.push_back(make_shared<ifstream>(path.toStdString()));

		std::shared_ptr<std::ifstream> f = myFiles[0];
		// std::ifstream file = f;
		// if (!myFile.is_open())
		// 	throw std::runtime_error("Could not open file");
		std::string line, colData;
		vector<double> rowData;
		if (f->good())
		{
			// Extract the first line in the file
			std::getline(*f, line);

			// Create a stringstream from line
			std::stringstream ss(line);

			// Extract each column name
			while (std::getline(ss, colData, ','))
			{
				rowData.push_back(std::stod(colData));
			}
		}
		for (auto a : rowData)
		{
			std::cout << a << std::endl;
		}
		printf("inside file\n");
	}
	catch (...)
	{
		printf("Can't open the file.\n");
	}
}

vector<double> SpecificWorker::getNextValue(std::shared_ptr<std::ifstream> fileD)
{
	vector<double> personPoseData;
	// std::shared_ptr<std::ifstream> fileD = fData;
	std::string line, colData;
	vector<double> rowData;
	if (fileD->good())
	{
		// Extract the first line in the file
		std::getline(*fileD, line);

		// Create a stringstream from line
		std::stringstream ss(line);

		// Extract each column name
		while (std::getline(ss, colData, ','))
		{
			rowData.push_back(std::stod(colData));
		}
	}
	try
	{
		personPoseData.push_back(rowData[0]);
	}
	catch (...)
	{
		personPoseData.push_back(0);
	}
	try
	{
		personPoseData.push_back(rowData[1]);
	}
	catch (...)
	{
		personPoseData.push_back(0);
	}
	try
	{
		personPoseData.push_back(rowData[2]);
	}
	catch (...)
	{
		personPoseData.push_back(0);
	}
	try
	{
		personPoseData.push_back(rowData[4]);
	}
	catch (...)
	{
		personPoseData.push_back(0);
	}
	return (personPoseData);
}

void SpecificWorker::playButton()
{
	if (play_button->text() == "PLAY")
	{
		play_button->setText("PAUSE");
		playTimer.start(playPeriod->value());
	}
	else
	{
		play_button->setText("PLAY");
		playTimer.stop();
	}

	std::cout << "play button clicked" << std::endl;
	// QString temp_path = folderLoc->text();
	// std::string path = temp_path.toStdString();
	// extractCSV(folderLoc->text());
}
int SpecificWorker::initPersons(int personId)
{
	RoboCompInnerModelManager::Pose3D pose;
	pose.x = 0;
	pose.y = 0;
	pose.z = 0;
	pose.rx = 0.f;
	pose.ry = 0.f;
	pose.rz = 0.f;

	string translationy = "0";
	string meshname = "human01.3ds";
	string scale = "12";
	string rotationz = "3.1415926535";

	int id = personId;
	int personSymbolId = -1;
	//personMap.size() + 1;
	// avoid inserting same element twice
	// while (personMap.find(id) != personMap.end())
	// 	id++;

	// Include person in RCIS
	if (includeInRCIS(id, pose, meshname))
	{
		// Include person in AGM
		personSymbolId = includeInAGM(id, pose, meshname);
		// if (personSymbolId != -1)
		// {
		// 	// TPerson person;
		// 	// person.autoMovement = false;
		// 	// person.pose = pose;
		// 	// person.personSymbolId = personSymbolId;
		// 	// person.name = "person" + std::to_string(id);
		// 	// personMap.insert(std::pair<int, TPerson>(personSymbolId, person));
		// 	//include in comboBox
		// 	// person_cb->addItem(QString::number(personSymbolId));
		// 	// int index = person_cb->findText(QString::number(personSymbolId));
		// 	// person_cb->setCurrentIndex(index);

		// 	// int1_cb->addItem(QString::number(personSymbolId));
		// 	// int2_cb->addItem(QString::number(personSymbolId));
		// }
	}
	return personSymbolId;
}

bool SpecificWorker::includeInRCIS(int id, const RoboCompInnerModelManager::Pose3D &pose, std::string meshName)
{
	printf("includeInRCIS begins\n");
	std::string name = "person" + std::to_string(id);

	RoboCompInnerModelManager::meshType mesh;
	mesh.pose.x = 0;
	mesh.pose.y = QString::fromStdString("0").toFloat();
	mesh.pose.z = 0;
	mesh.pose.rx = 1.57079632679;
	mesh.pose.ry = 0;
	mesh.pose.rz = QString::fromStdString("3.1415926535").toFloat();

	mesh.scaleX = mesh.scaleY = mesh.scaleZ = QString::fromStdString("12").toFloat();
	mesh.render = 0;
	mesh.meshPath = "/home/robocomp/robocomp/components/robocomp-viriato/files/osgModels/" + meshName;

	try
	{
		innermodelmanager_proxy->addTransform(name, "static", "root", pose);
	}
	catch (InnerModelManagerError e)
	{
		if (e.err != ErrorType::NodeAlreadyExists)
		{
			printf("Can't create fake peson\n");
			return false;
		}
	}
	try
	{
		innermodelmanager_proxy->addMesh(name + "_mesh", name, mesh);
	}
	catch (InnerModelManagerError e)
	{
		if (e.err != ErrorType::NodeAlreadyExists)
		{
			printf("Can't create fake peson\n");
			return false;
		}
	}
	printf("includeInRCIS ends\n");
	return true;
}

int SpecificWorker::includeInAGM(int id, const RoboCompInnerModelManager::Pose3D &pose, std::string mesh)
{
	printf("includeInAGM begins\n");

	std::string name = "person";
	std::string imName = "person" + std::to_string(id);
	int personSymbolId = -1;
	int idx = 0;
	while ((personSymbolId = worldModel->getIdentifierByType(name, idx++)) != -1)
	{
		printf("%d %d\n", idx, personSymbolId);
		if (worldModel->getSymbolByIdentifier(personSymbolId)->getAttribute("imName") == imName)
		{
			printf("found %d!!\n", personSymbolId);
			break;
		}
	}
	if (personSymbolId != -1)
	{
		printf("Fake person already in the AGM model\n");
		return personSymbolId;
	}

	AGMModel::SPtr newModel(new AGMModel(worldModel));

	// Symbolic part
	AGMModelSymbol::SPtr person = newModel->newSymbol("person");
	personSymbolId = person->identifier;
	printf("Got personSymbolId: %d\n", personSymbolId);
	person->setAttribute("imName", imName);
	person->setAttribute("imType", "transform");
	newModel->addEdgeByIdentifiers(person->identifier, 3, "in");

	// Geometric part
	std::map<std::string, std::string> edgeRTAtrs;
	edgeRTAtrs["tx"] = std::to_string(pose.x);
	edgeRTAtrs["ty"] = "0";
	edgeRTAtrs["tz"] = std::to_string(pose.z);
	edgeRTAtrs["rx"] = "0";
	edgeRTAtrs["ry"] = std::to_string(pose.ry);
	edgeRTAtrs["rz"] = "0";
	newModel->addEdgeByIdentifiers(100, person->identifier, "RT", edgeRTAtrs);

	AGMModelSymbol::SPtr personMesh = newModel->newSymbol("mesh_" + name);
	printf("personMesh %d\n", personMesh->identifier);
	personMesh->setAttribute("collidable", "false");
	personMesh->setAttribute("imName", imName + "_Mesh");
	personMesh->setAttribute("imType", "mesh");
	std::string meshPath = "/home/robocomp/robocomp/components/robocomp-viriato/files/osgModels/" + mesh;
	personMesh->setAttribute("path", meshPath);
	personMesh->setAttribute("render", "NormalRendering");
	personMesh->setAttribute("scalex", "12");
	personMesh->setAttribute("scaley", "12");
	personMesh->setAttribute("scalez", "12");

	edgeRTAtrs["tx"] = "0";
	edgeRTAtrs["ty"] = "0";
	edgeRTAtrs["tz"] = "0";
	edgeRTAtrs["rx"] = "1.570796326794";
	edgeRTAtrs["ry"] = "0";
	edgeRTAtrs["rz"] = "3.1415926535";
	newModel->addEdge(person, personMesh, "RT", edgeRTAtrs);

	while (true)
	{
		if (sendModificationProposal(worldModel, newModel))
		{
			break;
		}
		sleep(1);
	}
	printf("includeInAGM ends\n");
	return personSymbolId;
}
void SpecificWorker::compute()
{
	//computeCODE
	//QMutexLocker locker(mutex);
	//try
	//{
	//  camera_proxy->getYImage(0,img, cState, bState);
	//  memcpy(image_gray.data, &img[0], m_width*m_height*sizeof(uchar));
	//  searchTags(image_gray);
	//}
	//catch(const Ice::Exception &e)
	//{
	//  std::cout << "Error reading from Camera" << e << std::endl;
	//}
}

bool SpecificWorker::AGMCommonBehavior_activateAgent(const ParameterMap &prs)
{
	//implementCODE
	bool activated = false;
	if (setParametersAndPossibleActivation(prs, activated))
	{
		if (not activated)
		{
			return activate(p);
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool SpecificWorker::AGMCommonBehavior_deactivateAgent()
{
	//implementCODE
	return deactivate();
}

ParameterMap SpecificWorker::AGMCommonBehavior_getAgentParameters()
{
	//implementCODE
	return params;
}

StateStruct SpecificWorker::AGMCommonBehavior_getAgentState()
{
	//implementCODE
	StateStruct s;
	if (isActive())
	{
		s.state = RoboCompAGMCommonBehavior::StateEnum::Running;
	}
	else
	{
		s.state = RoboCompAGMCommonBehavior::StateEnum::Stopped;
	}
	s.info = p.action.name;
	return s;
}

void SpecificWorker::AGMCommonBehavior_killAgent()
{
	//implementCODE
}

bool SpecificWorker::AGMCommonBehavior_reloadConfigAgent()
{
	//implementCODE
	return true;
}

bool SpecificWorker::AGMCommonBehavior_setAgentParameters(const ParameterMap &prs)
{
	//implementCODE
	bool activated = false;
	return setParametersAndPossibleActivation(prs, activated);
}

int SpecificWorker::AGMCommonBehavior_uptimeAgent()
{
	//implementCODE
	return 0;
}

//SUBSCRIPTION to edgeUpdated method from AGMExecutiveTopic interface
void SpecificWorker::AGMExecutiveTopic_edgeUpdated(const RoboCompAGMWorldModel::Edge &modification)
{
	//subscribesToCODE
	QMutexLocker locker(mutex);
	AGMModelConverter::includeIceModificationInInternalModel(modification, worldModel);
	AGMInner::updateImNodeFromEdge(worldModel, modification, innerModel.get());
}

//SUBSCRIPTION to edgesUpdated method from AGMExecutiveTopic interface
void SpecificWorker::AGMExecutiveTopic_edgesUpdated(const RoboCompAGMWorldModel::EdgeSequence &modifications)
{
	//subscribesToCODE
	QMutexLocker lockIM(mutex);
	for (auto modification : modifications)
	{
		AGMModelConverter::includeIceModificationInInternalModel(modification, worldModel);
		AGMInner::updateImNodeFromEdge(worldModel, modification, innerModel.get());
	}
}

//SUBSCRIPTION to selfEdgeAdded method from AGMExecutiveTopic interface
void SpecificWorker::AGMExecutiveTopic_selfEdgeAdded(const int nodeid, const string &edgeType, const RoboCompAGMWorldModel::StringDictionary &attributes)
{
	//subscribesToCODE
	QMutexLocker lockIM(mutex);
	try
	{
		worldModel->addEdgeByIdentifiers(nodeid, nodeid, edgeType, attributes);
	}
	catch (...)
	{
		printf("Couldn't add an edge. Duplicate?\n");
	}

	try
	{
		innerModel = std::make_shared<InnerModel>(AGMInner::extractInnerModel(worldModel));
	}
	catch (...)
	{
		printf("Can't extract an InnerModel from the current model.\n");
	}
}

//SUBSCRIPTION to selfEdgeDeleted method from AGMExecutiveTopic interface
void SpecificWorker::AGMExecutiveTopic_selfEdgeDeleted(const int nodeid, const string &edgeType)
{
	//subscribesToCODE
	QMutexLocker lockIM(mutex);
	try
	{
		worldModel->removeEdgeByIdentifiers(nodeid, nodeid, edgeType);
	}
	catch (...)
	{
		printf("Couldn't remove an edge\n");
	}

	try
	{
		innerModel = std::make_shared<InnerModel>(AGMInner::extractInnerModel(worldModel));
	}
	catch (...)
	{
		printf("Can't extract an InnerModel from the current model.\n");
	}
}

//SUBSCRIPTION to structuralChange method from AGMExecutiveTopic interface
void SpecificWorker::AGMExecutiveTopic_structuralChange(const RoboCompAGMWorldModel::World &w)
{
	//subscribesToCODE
	mutex->lock();
	QMutexLocker lockIM(mutex);
	AGMModelConverter::fromIceToInternal(w, worldModel);

	innerModel = std::make_shared<InnerModel>(AGMInner::extractInnerModel(worldModel));
	mutex->unlock();
}

//SUBSCRIPTION to symbolUpdated method from AGMExecutiveTopic interface
void SpecificWorker::AGMExecutiveTopic_symbolUpdated(const RoboCompAGMWorldModel::Node &modification)
{
	//subscribesToCODE
	QMutexLocker locker(mutex);
	AGMModelConverter::includeIceModificationInInternalModel(modification, worldModel);
}

//SUBSCRIPTION to symbolsUpdated method from AGMExecutiveTopic interface
void SpecificWorker::AGMExecutiveTopic_symbolsUpdated(const RoboCompAGMWorldModel::NodeSequence &modifications)
{
	//subscribesToCODE
	QMutexLocker l(mutex);
	for (auto modification : modifications)
		AGMModelConverter::includeIceModificationInInternalModel(modification, worldModel);
}

bool SpecificWorker::setParametersAndPossibleActivation(const ParameterMap &prs, bool &reactivated)
{
	printf("<<< setParametersAndPossibleActivation\n");
	// We didn't reactivate the component
	reactivated = false;

	// Update parameters
	params.clear();
	for (ParameterMap::const_iterator it = prs.begin(); it != prs.end(); it++)
	{
		params[it->first] = it->second;
	}

	try
	{
		action = params["action"].value;
		std::transform(action.begin(), action.end(), action.begin(), ::tolower);
		//TYPE YOUR ACTION NAME
		if (action == "actionname")
		{
			active = true;
		}
		else
		{
			active = true;
		}
	}
	catch (...)
	{
		printf("exception in setParametersAndPossibleActivation %d\n", __LINE__);
		return false;
	}

	// Check if we should reactivate the component
	if (active)
	{
		active = true;
		reactivated = true;
	}

	printf("setParametersAndPossibleActivation >>>\n");

	return true;
}

bool SpecificWorker::sendModificationProposal(AGMModel::SPtr &worldModel, AGMModel::SPtr &newModel)
{
	bool result = false;
	try
	{
		AGMMisc::publishModification(newModel, agmexecutive_proxy, "HumanSceneSimAgent");
		result = true;
	}
	/*	catch(const RoboCompAGMExecutive::Locked &e)
	{
	}
	catch(const RoboCompAGMExecutive::OldModel &e)
	{
	}
	catch(const RoboCompAGMExecutive::InvalidChange &e)
	{
	}
*/
	catch (const Ice::Exception &e)
	{
		exit(1);
	}
	return result;
}