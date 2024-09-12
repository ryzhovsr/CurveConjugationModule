#include "RPLM.CAD.UI.СonjugationCurvesCommand.h"
#include "RPLM.Sample.UI.Resources.h"
#include "RPLM.Sample.ConjugateCurves.h"
#include "RPLM.Sample.Utils.h"
#include "Model/Filters/RGPSelectionFilter.h"
#include "Model/RGPSession.h"	
#include "Generators/WireBodyCreator.h"	
#include "Model/Data/GeometryData.h" 
#include "Model/Objects/RGPDimension.h"
#include <Graphics/RGPGraphicsForward.h>
#include "RPLM.UI.Widgets.MainWindow.h"
#include "RPLM.Shell.UI.MainWindowManager.h"
#include "RPLM.EP.Model/Model/Document/RGPMarkContainer.h"
#include "RPLM.EP.Common/Debug/RGPDebug.h"
#include <qdockwidget.h>
#include <qwidget.h>
#include <string>

#define RSCADUIW(key)	RPLM::Base::Framework::GetModuleResource(L##key, L"RPLM.CAD.Sample")

namespace RPLM::CAD
{
	namespace UI
	{
		RPLMCADСonjugationCurvesCommand::RPLMCADСonjugationCurvesCommand() :
			_selectObjectControl(L"SelectObjectControl", RSCADUIW("Object")),
			_groupCurveParameters(RSCADUIW("GroupCurveParameters"), L"GroupCurveParameters"),
			_editControlCurveDegree(L"EditControlCurveDegree", RSCADUIW("EditControlCurveDegree")),
			_readFromFileControlPoints(L"ReadFromFileControlPoints", RSCADUIW("ReadFromFileControlPoints"), L""),
			_readFromFileKnots(L"ReadFromFileKnots", RSCADUIW("ReadFromFileKnots"), L""),
			_buttonControlFixDerivatives(L"ButtonControlFixDerivatives", RSCADUIW("ButtonControlFixDerivatives"), L"", false, true),
			_fixOrderFirstDeriv(L"FixOrderFirstDeriv", RSCADUIW("FixOrderFirstDeriv"), false, false, false),
			_fixOrderLastDeriv(L"FixOrderLastDeriv", RSCADUIW("FixOrderLastDeriv"), false, false, false)
		{
			_dialog.SetTitle(RSCADUIW("RPLM.CAD.UI.ConjugationCurves"));

			AddOkToDialog(&_dialog);
			AddCancelToDialog(&_dialog);

			// Контрол "Выбрать объект"
			_selectObjectControl.SetPlaceholderText(RSCADUIW("SelectObject"));
			_dialog.AddControl(_selectObjectControl);

			_filter = std::make_shared<DimensionSelectionFilter>();
			_selected = std::make_shared<EP::Model::SelectionContainer>(GetDocument().get());

			// Степень кривой
			_groupCurveParameters.AddControl(_editControlCurveDegree);

			// Контрольные точки
			_groupCurveParameters.AddControl(_readFromFileControlPoints);
			_readFromFileControlPoints.SelectFile();

			// Узловой вектор
			_groupCurveParameters.AddControl(_readFromFileKnots);
			_readFromFileKnots.SelectFile();

			_dialog.AddControl(_groupCurveParameters);

			// Чекбокс фиксации производных
			_dialog.AddControl(_buttonControlFixDerivatives);

			// Порядок первой производной
			_groupFixOrderDerivs.AddControl(_fixOrderFirstDeriv);
			// Порядок последней производной
			_groupFixOrderDerivs.AddControl(_fixOrderLastDeriv);

			_groupFixOrderDerivs.SetHidden(true);
			_dialog.AddControl(_groupFixOrderDerivs);

			_dialog.OnCloseEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnCloseDialog, this);
			_ok.PressEvent = std::bind(&RPLMCADСonjugationCurvesCommand::onOK, this, std::placeholders::_1);
			_selectObjectControl.ClearObjectEvent = std::bind(&RPLMCADСonjugationCurvesCommand::onClearSelectObjectControl, this, std::placeholders::_1);
			_selectObjectControl.FocusSetEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnFocusSelectObjectControl, this, std::placeholders::_1);
			_buttonControlFixDerivatives.PressEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnFixateDerivates, this, std::placeholders::_1);
			
			
			
			/// Получить путь к виду, на котором выполняется команда
			///<returns>Путь (иерархия) видов, на котором выполняется команда</returns>
			//RPLM::EP::Model::ObjectVector GetPathToLayout() { return _pathToLayout; }
			
			//_singleObjectElement.ClearObjectEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnDischargeSingleObjectElement, this, std::placeholders::_1);
			//_singleObjectElement.FocusSetEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnFocusObjectElement, this, std::placeholders::_1);
			//_layoutControl.LayoutSelectedEvent = std::bind(&AddDimensionCommand::OnLayoutSelected, this, std::placeholders::_1);
		}

		RPLMCADСonjugationCurvesCommand::~RPLMCADСonjugationCurvesCommand()
		{

		}

		void RPLMCADСonjugationCurvesCommand::onClearSelectObjectControl(EP::UI::SingleObjectControl& iControl)
		{
			_selectObjectControl.Clear();
			EP::Model::MarkContainer* container = GetMarkContainer();
			container->RemoveAll(true);

			CheckOKButton();
		}

		void RPLMCADСonjugationCurvesCommand::OnFixateDerivates(EP::UI::ButtonControl& iControl)
		{
			_groupFixOrderDerivs.SetHidden(!_groupFixOrderDerivs.IsHidden());
		}

		bool RPLMCADСonjugationCurvesCommand::Start(EP::UI::StartCommandParameters& iParameters)
		{
			if (!Command::Start(iParameters))
			{
				return false;
			}
			
			CreateCommandDialog(_dialog, GetMainWindow(), GetDocument());
			_dialog.NeedToAdjust();
			_dialog.Show();

			CheckOKButton();

			return true;
		}

		void RPLMCADСonjugationCurvesCommand::Finish()
		{
			_dialog.Destroy();
			SetPrompt(GetView(), _STR(""));
			EmptyDynamicGraphics(GetView());
			Command::Finish();
			EP::Model::Session::GetSession()->GetExternal().DeleteUnusedData();
		}

		void RPLMCADСonjugationCurvesCommand::OnFocusObjectElement(EP::UI::SingleObjectControl& iControl)
		{
			_selectObjectControl.SetActive(false);
		}

		void RPLMCADСonjugationCurvesCommand::OnFocusSelectObjectControl(EP::UI::SingleObjectControl& iControl)
		{
			_selectObjectControl.SetActive(true);
		}

		void RPLMCADСonjugationCurvesCommand::OnDischargeSingleObjectElement(EP::UI::SingleObjectControl& iControl)
		{
			CheckOKButton();

			EP::Model::MarkContainer* container = GetMarkContainer();
			container->RemoveAll(true);
		}

		void RPLMCADСonjugationCurvesCommand::onOK(EP::UI::ButtonControl& iControl)
		{
			RGK::Context rgkContext;
			EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

			if (_selectedCurve)
			{
				if (auto path = std::make_unique<EP::Model::PathToObject>(EP::Model::PathToObject(_selectedCurve)))
				{
					auto objSelection = EP::UI::Command::CreateObjectSelectionFromPath(*path);

					if (auto parentObj = path->GetParentObject())
					{
						if (auto topologyObj = std::static_pointer_cast<RPLM::EP::Model::TopologyObject>(parentObj))
						{
							auto curve = dynamic_cast<EP::Model::CurveData*>(topologyObj.get());
							auto o = curve->GetCurve();
							auto link = topologyObj->GetLink();

							// ERROP auto data = link->GetTopologyData();

							// DO SOMETHING
						}
					}
				}

				
				//EP::Model::PathToObjectPtr path
				//RGK::NURBSCurve origiganalCurve(_selectedCurve);
			}
			else
			{
				auto controlPointsFile = _readFromFileControlPoints.GetFullName();
				auto kntotsFile = _readFromFileKnots.GetFullName();
				int degree = _editControlCurveDegree.GetIntValue();

				if (controlPointsFile.empty() || kntotsFile.empty() || degree == 0)
				{
					return;
				}

				RGK::Vector<RGK::Math::Vector3D> controlPoints = Sample::Utils::readControlPointsFromFile(controlPointsFile);
				Math::Geometry2D::Geometry::DoubleArray knots = Sample::Utils::readKnotsFromFile(kntotsFile);

				RGK::NURBSCurve origiganalCurve;
				RGK::NURBSCurve::Create(rgkContext, controlPoints, degree, knots, false, origiganalCurve);

				// Выполнение сопряжения исходной кривой с фиксацией производных
				RGK::NURBSCurve conjugatedCurve = ConjugateMethods::conjugateCurve(origiganalCurve, _fixOrderFirstDeriv.GetIntValue(), _fixOrderLastDeriv.GetIntValue());

				// Записываем контрольные точки новой кривой в файл
				Sample::Utils::writeControlPointsInFile(_STRING("C:\\Work\\rplm.all\\src\\SampleRPLM\\TempFile.txt"), conjugatedCurve.GetControlPoints());
			}
			
			Terminate();
		}

		void RPLMCADСonjugationCurvesCommand::CheckOKButton()
		{
			SetOKEnabled(_selectObjectControl.HasObject());
		}

		bool RPLMCADСonjugationCurvesCommand::OnCloseDialog()
		{
			Terminate();
			return false;
		}

		void RPLMCADСonjugationCurvesCommand::OnLayoutSelected(const RPLM::EP::Model::ObjectVector& iPathToLayout)
		{
			/*if (GetPathToLayout() == iPathToLayout)
			{
				if (_manipulator)
					_manipulator->SetPathToLayout(iPathToLayout);
				return;
			}

			bool activate = false;
			if (_active)
			{
				if (GetCommand() && GetCommand()->GetView())
				{
					if (auto layout = GetCommand()->GetView()->GetActiveLayout())
					{
						if (!iPathToLayout.empty() && iPathToLayout.front() != layout)
						{
							activate = true;
						}
					}
					else
						activate = true;
				}
			}

			SetPathToLayout(iPathToLayout, activate);
			if (_manipulator)
				_manipulator->SetPathToLayout(iPathToLayout);*/
		}
		
		RPLM::EP::Model::ObjectSelectionPtr RPLMCADСonjugationCurvesCommand::SelectObject(RPLM::EP::UI::SelectObjectParameters& iParameters)
		{
			auto selection = iParameters.Selection();

			if (auto objModel = std::dynamic_pointer_cast<RPLM::EP::Model::Object>(selection->GetObject()))
			{
				_selectObjectControl.SetObject(objModel);
				_selectedCurve = objModel;
			}

			CheckOKButton();

			return selection;
		}

		RPLM::EP::Model::SelectionFilterPtr RPLMCADСonjugationCurvesCommand::GetFilter() const
		{
			return _filter;
		}
							
		RPLM::EP::Model::ObjectSelectionPtr DimensionSelectionFilter::Select(const RPLM::EP::Model::ObjectSelectionPtr& iSelectionObject)const
		{
			RPLM::EP::Model::ObjectPtr obj = iSelectionObject->GetObject();
			RPLM::EP::Model::ObjectPtr objModel = std::dynamic_pointer_cast<RPLM::EP::Model::Object>(obj);
			if (objModel != nullptr)
				return iSelectionObject;
			return nullptr;
		}
	}
}