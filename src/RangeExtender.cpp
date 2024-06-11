#include "plugin.hpp"
#include <iostream>

struct RangeExtender : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUT3_OUTPUT,
		OUT4_OUTPUT,
		OUT5_OUTPUT,
		OUT6_OUTPUT,
		OUT7_OUTPUT,
		OUT8_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		BLINK1_LIGHT,
		BLINK2_LIGHT,
		BLINK3_LIGHT,
		BLINK4_LIGHT,
		BLINK5_LIGHT,
		BLINK6_LIGHT,
		BLINK7_LIGHT,
		BLINK8_LIGHT,
		LIGHTS_LEN
	};

	RangeExtender() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(INPUT_INPUT, "Input");
		configOutput(OUT1_OUTPUT, "Output 1");
		configOutput(OUT2_OUTPUT, "Output 2");
		configOutput(OUT3_OUTPUT, "Output 3");
		configOutput(OUT4_OUTPUT, "Output 4");
		configOutput(OUT5_OUTPUT, "Output 5");
		configOutput(OUT6_OUTPUT, "Output 6");
		configOutput(OUT7_OUTPUT, "Output 7");
		configOutput(OUT8_OUTPUT, "Output 8");
	}

	float clampLerp(float v, float min, float max) {
		return (v < min) ? 0.0f : (v > max) ? 10.0f : (v - min) / (max - min) * 10.0f;
	};

	void process(const ProcessArgs& args) override {
		float source_voltage = inputs[INPUT_INPUT].getVoltage();

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
				float min = 10.0 * (range_step) / number_of_outputs;
				float max = 10.0 * (range_step + 1) / number_of_outputs;
				float out = clampLerp(source_voltage, min, max);
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

		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(15.0, 20.0)), module, RangeExtender::INPUT_INPUT));

		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 30.0)),  module, RangeExtender::OUT1_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 42.0)),  module, RangeExtender::OUT2_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 54.0)),  module, RangeExtender::OUT3_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 66.0)),  module, RangeExtender::OUT4_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 78.0)),  module, RangeExtender::OUT5_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 90.0)),  module, RangeExtender::OUT6_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 102.0)), module, RangeExtender::OUT7_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(20.0, 114.0)), module, RangeExtender::OUT8_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 30.0)),  module, RangeExtender::BLINK1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 42.0)),  module, RangeExtender::BLINK2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 54.0)),  module, RangeExtender::BLINK3_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 66.0)),  module, RangeExtender::BLINK4_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 78.0)),  module, RangeExtender::BLINK5_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 90.0)),  module, RangeExtender::BLINK6_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 102.0)), module, RangeExtender::BLINK7_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 114.0)), module, RangeExtender::BLINK8_LIGHT));
	}
};


Model* modelRangeExtender = createModel<RangeExtender, RangeExtenderWidget>("RangeExtender");
