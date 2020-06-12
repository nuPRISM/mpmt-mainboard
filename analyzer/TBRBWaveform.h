#ifndef TBRBWaveform_h
#define TBRBWaveform_h

#include <string>
#include "THistogramArrayBase.h"

/// Class for making histograms of raw BRB waveforms;
class TBRBWaveform : public THistogramArrayBase {
public:
  TBRBWaveform();
  virtual ~TBRBWaveform(){std::cout << "TBRBWaveform destructor" << std::endl;};

  void UpdateHistograms(TDataContainer& dataContainer);

  // Reset the histograms; needed before we re-fill each histo.
  void Reset();
  
  void CreateHistograms();
  
  /// Take actions at begin run
  void BeginRun(int transition,int run,int time){		
    CreateHistograms();		
  }

private:

  int FrequencySetting;
};



/// Class for making histograms of raw BRB waveforms;
class TBRBBaseline : public THistogramArrayBase {
public:
  TBRBBaseline();
  virtual ~TBRBBaseline(){};

  void UpdateHistograms(TDataContainer& dataContainer);

  void CreateHistograms();
  
  /// Take actions at begin run
  void BeginRun(int transition,int run,int time){		
    CreateHistograms();		
  }

};

/// Class for making histograms of raw BRB waveforms;
class TBRBRMS : public THistogramArrayBase {
public:
  TBRBRMS();
  virtual ~TBRBRMS(){};

  void UpdateHistograms(TDataContainer& dataContainer);

  void CreateHistograms();
  
  /// Take actions at begin run
  void BeginRun(int transition,int run,int time){		
    CreateHistograms();		
  }

};


/// Class for making histograms of pulseheights;
class TBRBPH : public THistogramArrayBase {
public:
  TBRBPH();
  virtual ~TBRBPH(){};

  void UpdateHistograms(TDataContainer& dataContainer);

  void CreateHistograms();
  
  /// Take actions at begin run
  void BeginRun(int transition,int run,int time){ CreateHistograms(); }

};

#endif


