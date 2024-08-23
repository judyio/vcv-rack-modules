#include "plugin.hpp"
#include <iostream>

struct RangeExtender : Module {
	enum ParamId {
		PARAM_OVERLAP,
		PARAM_SMOOTH,
		PARAMS_LEN
	};
	enum InputId {
		INPUT_INPUT,
		INPUT_OVERLAP,
		INPUT_SMOOTH,
		INPUTS_LEN
	};
	enum OutputId {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUT3_OUTPUT,
		OUT4_OUTPUT,
		OUT5_OUTPUT,
		OUT6_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		BLINK1_LIGHT,
		BLINK2_LIGHT,
		BLINK3_LIGHT,
		BLINK4_LIGHT,
		BLINK5_LIGHT,
		BLINK6_LIGHT,
		LIGHTS_LEN
	};

	RangeExtender() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(INPUT_INPUT, "Input");
		configParam(PARAM_OVERLAP, 0.0f, 100.0f, 0.0f, "Control output overlap directly", "%");
		configInput(INPUT_OVERLAP, "Output overlap");
		configParam(PARAM_SMOOTH, 0.0f, 100.0f, 0.0f, "Control smoothness directly", "%");
		configInput(INPUT_SMOOTH, "Output smoothness");
		configOutput(OUT1_OUTPUT, "Output 1");
		configOutput(OUT2_OUTPUT, "Output 2");
		configOutput(OUT3_OUTPUT, "Output 3");
		configOutput(OUT4_OUTPUT, "Output 4");
		configOutput(OUT5_OUTPUT, "Output 5");
		configOutput(OUT6_OUTPUT, "Output 6");
	}

	float smoothlyClampLerpToTen(float v, float min, float max, float smooth) {
		// refactor to simplify
		if (v < min) {
			return 0.0f;
		} else if (v > max) {
			return 10.0f;
		} else {
			float linear_response = (v - min) / (max - min);
			float smooth_response = smootherstep(linear_response);
			return percentageLerp(smooth, linear_response, smooth_response) * 10.0f;
		}
	};

	float basicLerp(float v, float inmin, float inmax, float outmin, float outmax) {
		return outmin + ((outmax - outmin) * ((v - inmin) / (inmax - inmin)));
	};

	float percentageLerp(float v, float outmin, float outmax) {
		return outmin + (outmax - outmin) * v;
	}

	// float smoothstep(float x) {
	// 	return x * x * (3.0f - 2.0f * x);
	// }

	float smootherstep(float x) {
		return x * x * x * (x * (6.0f * x - 15.0f) + 10.0f);
	}

	void process(const ProcessArgs& args) override {
		float source_voltage = inputs[INPUT_INPUT].getVoltage();

		// get overlap value
		float overlap;
		if (inputs[INPUT_OVERLAP].isConnected()) {
			overlap = inputs[INPUT_OVERLAP].getVoltage() / 10.0f;
		} else {
			overlap = params[PARAM_OVERLAP].getValue() / 100.0f;
		}

		// get smooth value
		float smooth;
		if (inputs[INPUT_SMOOTH].isConnected()) {
			smooth = inputs[INPUT_SMOOTH].getVoltage() / 10.0f;
		} else {
			smooth = params[PARAM_SMOOTH].getValue() / 100.0f;
		}

		// first loop, count number of plugged in outputs
		int number_of_outputs = 0;
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			if (outputs[OutputId(i)].isConnected()) {
				number_of_outputs += 1;
			}
		}

		// second loop, based on number of outputs, set the voltage
		int range_step = 0;
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			if (outputs[OutputId(i)].isConnected()) {
				float min = 10.0 * percentageLerp(overlap, range_step, 0.0) / number_of_outputs;
				float max = 10.0 * percentageLerp(overlap, (range_step + 1), number_of_outputs) / number_of_outputs;
				float out = smoothlyClampLerpToTen(source_voltage, min, max, smooth);
				outputs[OutputId(i)].setVoltage(out);
				lights[LightId(i)].setBrightness(out / 10.0f);
				range_step++;
			} else {
				outputs[OutputId(i)].setVoltage(0.0f);
				lights[LightId(i)].setBrightness(0.0f);
			}
		}
	}
};


struct RangeExtenderWidget : ModuleWidget {
	RangeExtenderWidget(RangeExtender* module) {
		setModule(module);
		setPanel(createPanel(
			asset::plugin(pluginInstance, "res/RangeExtender.svg"),
			asset::plugin(pluginInstance, "res/RangeExtenderDark.svg")
		));

		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(15.0, 20.0)),  module, RangeExtender::INPUT_INPUT));
		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 30.0 )), module, RangeExtender::INPUT_OVERLAP));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.0, 30.0)), module, RangeExtender::PARAM_OVERLAP));
		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 42.0 )), module, RangeExtender::INPUT_SMOOTH));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.0, 42.0)), module, RangeExtender::PARAM_SMOOTH));

		// addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 30.0 )),  module, RangeExtender::OUT1_OUTPUT));
		// addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 42.0 )),  module, RangeExtender::OUT2_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 54.0 )), module, RangeExtender::OUT1_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 66.0 )), module, RangeExtender::OUT2_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 78.0 )), module, RangeExtender::OUT3_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 90.0 )), module, RangeExtender::OUT4_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 102.0)), module, RangeExtender::OUT5_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 114.0)), module, RangeExtender::OUT6_OUTPUT));

		// addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 30.0 )),  module, RangeExtender::BLINK1_LIGHT));
		// addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 42.0 )),  module, RangeExtender::BLINK2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 54.0 )), module, RangeExtender::BLINK1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 66.0 )), module, RangeExtender::BLINK2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 78.0 )), module, RangeExtender::BLINK3_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 90.0 )), module, RangeExtender::BLINK4_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 102.0)), module, RangeExtender::BLINK5_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 114.0)), module, RangeExtender::BLINK6_LIGHT));
	}
};


Model* modelRangeExtender = createModel<RangeExtender, RangeExtenderWidget>("RangeExtender");
