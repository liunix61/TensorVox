#include "tacotron2.h"

Tacotron2::Tacotron2()
{

}

TFTensor<float> Tacotron2::DoInference(const std::vector<int32_t> &InputIDs, const std::vector<float> &ArgsFloat, const std::vector<int32_t> ArgsInt, int32_t SpeakerID, int32_t EmotionID)
{
    if (!CurrentMdl)
        throw std::exception("Tried to do inference on unloaded or invalid model!");




    QString dastr;
    for (int32_t fish : InputIDs)
        dastr += QString::number(fish) + " ";



    // Convenience reference so that we don't have to constantly derefer pointers.
    cppflow::model& Mdl = *CurrentMdl;


    // Define the tensors

    // This is the shape of the input IDs, our equivalent to tf.expand_dims.
    std::vector<int64_t> InputIDShape = { 1, (int64_t)InputIDs.size() };



    cppflow::tensor input_ids{ InputIDs, InputIDShape };
    cppflow::tensor speaker_ids{SpeakerID };
    cppflow::tensor input_lengths{(int32_t)InputIDs.size() };
    cppflow::tensor* emotion_ids = nullptr;


    // This is a multi-emotion model
    if (EmotionID != -1)
    {
        emotion_ids = new cppflow::tensor{std::vector<int32_t>{EmotionID}};

    }

    TensorVec Inputs = {{"serving_default_input_ids:0",input_ids},
                        {"serving_default_input_lengths:0",input_lengths},
                        {"serving_default_speaker_ids:0",speaker_ids}};



    // Define output tensor
    if (EmotionID != -1)
        Inputs.push_back({"serving_default_emotion_ids:0",emotion_ids});


    // Do inference

    auto Outputs = Mdl(Inputs,{"StatefulPartitionedCall:0","StatefulPartitionedCall:1","StatefulPartitionedCall:2"});

    // Define output and return it
    TFTensor<float> Output = VoxUtil::CopyTensor<float>(Outputs[1]);

    // We allocated the emotion_ids cppflow::tensor dynamically, delete it
    if (emotion_ids)
        delete emotion_ids;

    // We could just straight out define it in the return statement, but I like it more this way

    return Output;

}
