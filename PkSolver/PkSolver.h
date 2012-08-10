/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: pk_solver.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.13 $

=========================================================================auto=*/

#ifndef PkSolver_h_
#define PkSolver_h_

#include "itkLevenbergMarquardtOptimizer.h"
#include <math.h>
#include <vnl/algo/vnl_convolve.h>
#include "itkArray.h"

namespace itk
{

class LMCostFunction: public itk::MultipleValuedCostFunction
{
public:
  typedef LMCostFunction                    Self;
  typedef itk::MultipleValuedCostFunction   Superclass;
  typedef itk::SmartPointer<Self>           Pointer;
  typedef itk::SmartPointer<const Self>     ConstPointer;
  itkNewMacro( Self );
        
  enum { SpaceDimension =  3 };
  unsigned int RangeDimension; 
        
  typedef Superclass::ParametersType              ParametersType;
  typedef Superclass::DerivativeType              DerivativeType;
  typedef Superclass::MeasureType                 MeasureType, ArrayType;
  typedef Superclass::ParametersValueType         ValueType;
		      
        
  float m_Hematocrit;
        
  LMCostFunction():
    m_Measure(700)
  {
  }
        
  void SetHematocrit (float hematocrit) {
    m_Hematocrit = hematocrit;
  }
        
  void SetNumberOfValues(unsigned int NumberOfValues)
  {
    RangeDimension = NumberOfValues;
  }
        
  void SetCb (const float* cb, int sz) //BloodConcentrationCurve.
  {
    double *tmp;
    tmp = new double[sz];
    Cb.set_size(sz);
    for( int i = 0; i < sz; ++i )
      tmp[i] = cb[i];
    Cb.set(tmp);
    delete [] tmp;
  }
        
        
  void SetCv (const float* cv, int sz) //Self signal Y
  {    
    double *tmp;
    tmp = new double[sz];
    Cv.set_size (sz);
    for (int i = 0; i < sz; ++i)
      tmp[i] = cv[i];
    Cv.set(tmp);
    delete [] tmp;
  }
        
  void SetTime (const float* cx, int sz) //Self signal X
  {
    double *tmp;
    tmp = new double[sz];
    Time.set_size (sz);
    for( int i = 0; i < sz; ++i )
      tmp[i] = cx[i];
    Time.set(tmp);
    delete [] tmp;
  }
        
  MeasureType GetValue( const ParametersType & parameters) const
  {
    m_Measure.SetSize(RangeDimension);
    ValueType Ktrans = parameters[0];
    ValueType Ve = parameters[1];
    ValueType f_pv = parameters[2];
            
    ArrayType VeTerm;
    VeTerm = -Ktrans/Ve*Time;
    ValueType deltaT = Time(1) - Time(0);
    m_Measure = Cv - (1/(1.0-m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb,Exponential(VeTerm)) + f_pv*Cb));
            
    return m_Measure; 
  }
        
  //Not going to be used
  void GetDerivative( const ParametersType & /* parameters*/,
                      DerivativeType  & /*derivative*/ ) const
  {   
  }
        
  unsigned int GetNumberOfParameters(void) const
  {
    return SpaceDimension;
  }
        
  unsigned int GetNumberOfValues(void) const
  {
    return RangeDimension;
  }
        
protected:
  virtual ~LMCostFunction(){}
private:
        
  mutable MeasureType       m_Measure;
  mutable DerivativeType    m_Derivative;
        
  ArrayType Cv, Cb, Time;
        
  ArrayType Convolution(ArrayType X, ArrayType Y) const
  {
    ArrayType Z;
    Z.set_size(X.size());
    ArrayType temp;
    temp = vnl_convolve(X,Y);
    Z = temp.extract(X.size(),0);
    return Z;
  };
        
  ArrayType Exponential(ArrayType X) const
  {
    ArrayType Z;
    Z.set_size(X.size());
    for (unsigned int i=0; i<X.size(); i++)
      {
      Z[i] = exp(X(i));
      }
    return Z;
  };
        
  int constraintFunc(ValueType x) const
  {
    if (x<0||x>1)
      return 1;
    else
      return 0;
            
  };
        
        
};
    
class CommandIterationUpdateLevenbergMarquardt : public itk::Command 
{
public:
  typedef  CommandIterationUpdateLevenbergMarquardt   Self;
  typedef  itk::Command                               Superclass;
  typedef itk::SmartPointer<Self>                     Pointer;
  itkNewMacro( Self );
protected:
  CommandIterationUpdateLevenbergMarquardt() 
  {
    m_IterationNumber=0;
  }
  virtual ~CommandIterationUpdateLevenbergMarquardt(){}
public:
  typedef itk::LevenbergMarquardtOptimizer   OptimizerType;
  typedef   const OptimizerType   *          OptimizerPointer;
        
  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *)caller, event);
  }
        
  void Execute(const itk::Object * object, const itk::EventObject & event)
  {
    //std::cout << "Observer::Execute() " << std::endl;
    OptimizerPointer optimizer = 
      dynamic_cast< OptimizerPointer >( object );
    if( m_FunctionEvent.CheckEvent( &event ) )
      {
      // std::cout << m_IterationNumber++ << "   ";
      // std::cout << optimizer->GetCachedValue() << "   ";
      // std::cout << optimizer->GetCachedCurrentPosition() << std::endl;
      }
    else if( m_GradientEvent.CheckEvent( &event ) )
      {
      std::cout << "Gradient " << optimizer->GetCachedDerivative() << "   ";
      }
            
  }
private:
  unsigned long m_IterationNumber;
        
  itk::FunctionEvaluationIterationEvent m_FunctionEvent;
  itk::GradientEvaluationIterationEvent m_GradientEvent;
};
  
bool pk_solver(int signalSize, const float* timeAxis, 
               const float* PixelConcentrationCurve, 
               const float* BloodConcentrationCurve, 
               float& Ktrans, float& Ve, float& Fpv,
               float fTol = 1e-4f, 
               float gTol = 1e-4f, 
               float xTol = 1e-5f,
               float epsilon = 1e-9f, 
               int maxIter = 200,
               float hematocrit = 0.4f);

bool pk_solver(int signalSize, const float* timeAxis, 
               const float* PixelConcentrationCurve, const float* BloodConcentrationCurve, 
               float& Ktrans, float& Ve, float& Fpv,
               float fTol, float gTol,float xTol,
               float epsilon, int maxIter, float hematocrit,
               itk::LevenbergMarquardtOptimizer* optimizer,
               LMCostFunction* costFunction
  );

void pk_report();
void pk_clear();

bool convert_signal_to_concentration (unsigned int signalSize, 
                                      const float* SignalIntensityCurve, 
                                      float T1, float TR, float FA,
                                      float* concentration,
                                      float relaxivity = 4.9E-3f,
                                      float s0 = -1.0f,
                                      float S0GradThresh = 15.0f);

float area_under_curve(int signalSize, const float* timeAxis,const float* concentration, int BATIndex, float aucTimeInterval);

float intergrate(float* yValues, float * xValues, int size);

void compute_derivative (int signalSize, const float* SingnalY, float* YDeriv);

void compute_derivative_forward (int signalSize, const float* SignalY, float* YDeriv);

void compute_derivative_backward (int signalSize, const float* SignalY, float* YDeriv);

float get_signal_max (int signalSize, const float* SignalY);

bool compute_bolus_arrival_time (int signalSize, const float* SignalY,
                                 int& ArrivalTime, int& FirstPeak, float& MaxSlope);

void compute_gradient (int signalSize, const float* SignalY, float* SignalGradient);

void compute_gradient_forward (int signalSize, const float* SignalY, float* SignalGradient);

void compute_gradient_backward (int signalSize, const float* SignalY, float* SignalGradient);

float compute_s0_using_sumsignal_properties (int signalSize, const float* SignalY, 
                                             const short* lowGradIndex, int FirstPeak);

float compute_s0_individual_curve (int signalSize, const float* SignalY, float S0GradThresh);

};

#endif
