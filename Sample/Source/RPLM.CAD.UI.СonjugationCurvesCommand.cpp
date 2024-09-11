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

#define RSCADUIW(key)				::RPLM::Base::Framework::GetModuleResource(L##key, L"RPLM.CAD.Sample")

namespace RPLM::CAD
{
	namespace UI
	{
		RPLMCADСonjugationCurvesCommand::RPLMCADСonjugationCurvesCommand() :
			_singleObject(L"SingleObject", RSCADUIW("SingleObject")),
			_layoutControl(L"Layout", RPLM::EP::Model::ObjectFilterPtr(), L"View"),
			_singleObjectElement(L"SingleObjectElement", RSCADUIW("SingleObjectElement")),
			_groupCurveParameters(RSCADUIW("GroupCurveParameters"), L"GroupCurveParameters"),
			_editValueCurveDegree(L"EditValueCurveDegree", RSCADUIW("EditValueCurveDegree")),
			_readFromFileControlPoints(L"ReadFromFileControlPoints", RSCADUIW("ReadFromFile"), L""),
			_readFromFileKnots(L"ReadFromFileKnots", RSCADUIW("ReadFromFile"), _STR("")),
			_buttonFixDerivatives(L"ButtonFixDerivatives", RSCADUIW("ButtonFixDerivatives"), L"", false, true),
			_fixOrderFirstDeriv(L"FixOrderFirstDeriv", RSCADUIW("FixOrderFirstDeriv"), false, false, false),
			_fixOrderLastDeriv(L"FixOrderLastDeriv", RSCADUIW("FixOrderLastDeriv"), false, false, false)
		{
			_filter = std::make_shared<DimensionSelectionFilter>();
			_selected = std::make_shared<RPLM::EP::Model::SelectionContainer>(GetDocument().get());

			_dialog.SetTitle(RSCADUIW("RPLM.CAD.UI.ConjugationCurves"));

			_dialog.AddOption(_ok);
			_dialog.AddOption(_cancel);	

			// Объект
			//_singleObject.SetPlaceholderText(_STRING("ObjectText"));
			//_dialog.AddControl(_singleObject);

			//// Вид
			//_layoutControl.AddValue(_STR(""));
			//_layoutControl.AddValue(RSCADUIW("ValueVidUp"), _STRING("SelectWorkplane"));
			//_layoutControl.AddValue(RSCADUIW("ValueVidLeft"), _STRING("SelectWorkplane"));
			//_layoutControl.AddValue(RSCADUIW("ValueVidTop"), _STRING("SelectWorkplane"));
			//_dialog.AddControl(_layoutControl);

			//// Степень кривой
			//_groupCurveParameters.AddControl(_editValueCurveDegree);

			//// Контрольные точки
			//_groupCurveParameters.AddControl(_readFromFileControlPoints);
			//_readFromFileControlPoints.SetTitle(_STR("Контрольные точки"));
			//_readFromFileControlPoints.SelectFile();

			//// Узловой вектор
			//_groupCurveParameters.AddControl(_readFromFileKnots);
			//_readFromFileKnots.SetTitle(_STR("Узловой вектор"));
			//_readFromFileKnots.SelectFile();

			//_dialog.AddControl(_groupCurveParameters);

			//// Чекбокс фиксации производных
			//_dialog.AddControl(_buttonFixDerivatives);

			//// Порядок первой производной
			//_groupOrderDerivs.AddControl(_fixOrderFirstDeriv);
			//// Порядок последней производной
			//_groupOrderDerivs.AddControl(_fixOrderLastDeriv);

			//_groupOrderDerivs.SetHidden(true);
			//_dialog.AddControl(_groupOrderDerivs);

			///// Получить путь к виду, на котором выполняется команда
			/////<returns>Путь (иерархия) видов, на котором выполняется команда</returns>
			////RPLM::EP::Model::ObjectVector GetPathToLayout() { return _pathToLayout; }

			_dialog.OnCloseEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnCloseDialog, this);
			_ok.PressEvent = std::bind(&RPLMCADСonjugationCurvesCommand::onOK, this, std::placeholders::_1);
			//_buttonFixDerivatives.PressEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnFixateDerivates, this, std::placeholders::_1);
			//_singleObject.ClearObjectEvent = std::bind(&RPLMCADСonjugationCurvesCommand::onDischargeSingleObject, this, std::placeholders::_1);
			//_singleObjectElement.ClearObjectEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnDischargeSingleObjectElement, this, std::placeholders::_1);
			//_singleObjectElement.FocusSetEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnFocusObjectElement, this, std::placeholders::_1);
			//_singleObject.FocusSetEvent = std::bind(&RPLMCADСonjugationCurvesCommand::OnFocusObject, this, std::placeholders::_1);
			////_layoutControl.LayoutSelectedEvent = std::bind(&AddDimensionCommand::OnLayoutSelected, this, std::placeholders::_1);
		}

		RPLMCADСonjugationCurvesCommand::~RPLMCADСonjugationCurvesCommand()
		{

		}

		void RPLMCADСonjugationCurvesCommand::onDischargeSingleObject(RPLM::EP::UI::SingleObjectControl& iControl)
		{
			_singleObject.Clear();
			RPLM::EP::Model::MarkContainer* container = GetMarkContainer();
			container->RemoveAll(true);
		}

		void RPLMCADСonjugationCurvesCommand::OnFixateDerivates(RPLM::EP::UI::ButtonControl& iControl)
		{
			_groupOrderDerivs.SetHidden(!_groupOrderDerivs.IsHidden());
		}

		bool RPLMCADСonjugationCurvesCommand::Start(RPLM::EP::UI::StartCommandParameters& iParameters)
		{
			if (!Command::Start(iParameters))
				return false;			
			auto document = GetDocument();
			CreateCommandDialog(_dialog, GetMainWindow(), document);
			_dialog.NeedToAdjust();
			_dialog.Show();
			return true;			
		}

		void RPLMCADСonjugationCurvesCommand::Finish()
		{
			_dialog.Destroy();
			SetPrompt(GetView(), _STR(""));
			EmptyDynamicGraphics(GetView());
			Command::Finish();

			RPLM::EP::Model::Session::GetSession()->GetExternal().DeleteUnusedData();
		}

		void RPLMCADСonjugationCurvesCommand::OnFocusObjectElement(RPLM::EP::UI::SingleObjectControl& iControl)
		{
			_singleObjectElement.SetActive(true);
			_singleObject.SetActive(false);
		}

		void RPLMCADСonjugationCurvesCommand::OnFocusObject(EP::UI::SingleObjectControl& iControl)
		{
			_singleObjectElement.SetActive(false);
			_singleObject.SetActive(true);
		}

		void RPLMCADСonjugationCurvesCommand::OnDischargeSingleObjectElement(EP::UI::SingleObjectControl& iControl)
		{
			_singleObjectElement.Clear();
			EP::Model::MarkContainer* container = GetMarkContainer();
			container->RemoveAll(true);
		}

		void RPLMCADСonjugationCurvesCommand::onOK(EP::UI::ButtonControl& iControl)
		{
			RGK::Context rgkContext;
			EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

			auto controlPointsFile = _readFromFileControlPoints.GetFullName();
			auto kntotsFile = _readFromFileKnots.GetFullName();
			int degree = _editValueCurveDegree.GetIntValue();

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
			Terminate();
		}

		void RPLMCADСonjugationCurvesCommand::OnCancel(FinishCommandContext& context)
		{
			Terminate();
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
			RPLM::EP::Model::ObjectPtr objModel = std::dynamic_pointer_cast<RPLM::EP::Model::Object>(selection->GetObject());
			if (objModel != nullptr && _singleObjectElement.IsActive() == false)
				_singleObject.SetObject(objModel);
			else 
				_singleObjectElement.SetObject(objModel);
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