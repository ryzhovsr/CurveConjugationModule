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
#include <RPLM.EP.Model/Model/Dependencies/RGPInterpolationSpline.h>

#define RSCADUIW(key)	RPLM::Base::Framework::GetModuleResource(L##key, L"RPLM.CAD.Sample")

namespace RPLM::CAD
{
	namespace UI
	{
		RPLMCADСonjugationCurvesCommand::RPLMCADСonjugationCurvesCommand() :
			_selectObjectControl(L"SelectObjectControl", RSCADUIW("Object")),
			_groupCurveParameters(RSCADUIW("GroupCurveParameters"), L"GroupCurveParameters"),
			_curveDegree(L"CurveDegree", RSCADUIW("CurveDegree")),
			_controlPoints(L"ControlPoints", RSCADUIW("ControlPoints"), L""),
			_knots(L"Knots", RSCADUIW("Knots"), L""),
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
			_groupCurveParameters.AddControl(_curveDegree);

			// Контрольные точки
			_groupCurveParameters.AddControl(_controlPoints);
			_controlPoints.SelectFile();

			// Узловой вектор
			_groupCurveParameters.AddControl(_knots);
			_knots.SelectFile();

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
			//bool typeSpline;
			//auto curve = std::static_pointer_cast<EP::Model::Curve2D>(_selectedCurve);
			//curve->GetCurveData().GetSplineInterpolationType(typeSpline);

			/*if (typeSpline && curve->GetDependencies())
			{
				EP::DoubleArray knots;

				for (auto dep : curve->GetDependencies().Get())
				{
					auto interpolationDep = std::static_pointer_cast<RPLM::EP::Model::Curve>(dep);

					if (interpolationDep == nullptr)
						continue;

					for (const auto knot : interpolationDep->GetKnots())
					{
						knots.push_back(knot->GetValue());
					}

					RGK::NURBSCurve origiganalCurve;
					RGK::Vector<RGK::Math::Vector3D> controlPoints{};
					RGK::NURBSCurve::Create(rgkContext, controlPoints, 3, {}, false, origiganalCurve);
				}
			}*/

			{
				// Получаем введённые значения из контролов
				Base::Framework::String controlPointsPath = _controlPoints.GetFullName();
				Base::Framework::String knotsPath = _knots.GetFullName();
				int degree = _curveDegree.GetIntValue();

				if (controlPointsPath.empty() || knotsPath.empty() || degree <= 0)
				{
					Terminate();
					return;
				}

				// Считываем данные из файлов
				RGK::Vector<RGK::Math::Vector3D> controlPoints = Sample::Utils::readControlPointsFromFile(controlPointsPath);
				Math::Geometry2D::Geometry::DoubleArray knots = Sample::Utils::readKnotsFromFile(knotsPath);

				RGK::Context rgkContext;
				EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

				// Создаём объект исходной кривой
				RGK::NURBSCurve origiganalCurve;
				RGK::NURBSCurve::Create(rgkContext, controlPoints, degree, knots, false, origiganalCurve);

				// Выполнение сопряжения исходной кривой с фиксацией производных
				RGK::NURBSCurve conjugatedCurve = Sample::ConjugationMethods::conjugateCurve(origiganalCurve, _fixOrderFirstDeriv.GetIntValue(), _fixOrderLastDeriv.GetIntValue());

				// Записываем контрольные точки новой кривой в файл
				Sample::Utils::writeControlPointsInFile(_STRING("C:\\Work\\rplm.all\\src\\SampleRPLM\\TempFile.txt"), conjugatedCurve.GetControlPoints());
			}
			
			Terminate();
		}

		void RPLMCADСonjugationCurvesCommand::CheckOKButton()
		{
			//SetOKEnabled(_selectObjectControl.HasObject());
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