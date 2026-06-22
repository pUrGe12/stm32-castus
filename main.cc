#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"

extern "C" { void uart_init(void); void uart_write(const char*); }
extern const unsigned char model_tflite[];
extern const unsigned int  model_tflite_len;

namespace {
constexpr int kArenaSize = 2048;
alignas(16) uint8_t arena[kArenaSize];
}

int main(){
  uart_init();
  RegisterDebugLogCallback(uart_write);

  const tflite::Model* model = tflite::GetModel(model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION){ MicroPrintf("schema mismatch"); for(;;); }

  tflite::MicroMutableOpResolver<1> resolver;
  resolver.AddFullyConnected();

  tflite::MicroInterpreter interpreter(model, resolver, arena, kArenaSize);
  if (interpreter.AllocateTensors() != kTfLiteOk){
    MicroPrintf("AllocateTensors FAILED (arena too small) -- boot aborted");
    for(;;);
  }
  MicroPrintf("boot ok, arena_used=%d", (int)interpreter.arena_used_bytes());

  TfLiteTensor* in  = interpreter.input(0);
  TfLiteTensor* out = interpreter.output(0);

  const float pi = 3.14159265f;
  for (int i=0;i<=20;i++){
    float x = (i/20.0f)*2.0f*pi;
    in->data.int8[0] = (int8_t)(x/in->params.scale + in->params.zero_point);
    if (interpreter.Invoke()!=kTfLiteOk){ MicroPrintf("invoke failed"); for(;;); }
    float y = (out->data.int8[0] - out->params.zero_point) * out->params.scale;
    MicroPrintf("x_milli=%d  y_milli=%d", (int)(x*1000.0f), (int)(y*1000.0f));
  }
  for(;;);
}