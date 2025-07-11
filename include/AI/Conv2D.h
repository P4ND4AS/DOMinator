#ifndef CONV2D_H
#define CONV2D_H

#include <Eigen/Dense>

class Conv2D {
public:
    // Convolution 2D avec padding et stride (mono-filtre, mono-canal)
    static Eigen::MatrixXf convolve2D(
        const Eigen::MatrixXf& input,
        const Eigen::MatrixXf& kernel,
        int stride_h = 1,
        int stride_w = 1,
        int padding = 0
    ) {
        int H = input.rows();
        int W = input.cols();
        int kH = kernel.rows();
        int kW = kernel.cols();

        Eigen::MatrixXf padded = Eigen::MatrixXf::Zero(H + 2 * padding, W + 2 * padding);
        padded.block(padding, padding, H, W) = input;

        int outH = (H + 2 * padding - kH) / stride_h + 1;
        int outW = (W + 2 * padding - kW) / stride_w + 1;

        Eigen::MatrixXf output(outH, outW);

        for (int i = 0; i < outH; ++i) {
            for (int j = 0; j < outW; ++j) {
                int row = i * stride_h;
                int col = j * stride_w;

                Eigen::MatrixXf patch = padded.block(row, col, kH, kW);
                output(i, j) = (patch.array() * kernel.array()).sum();
            }
        }

        return output;
    }

    static std::vector<Eigen::MatrixXf> convolveMultiFilter(
        const Eigen::MatrixXf& input,
        const std::vector<Eigen::MatrixXf>& kernels,
        int stride_h = 1,
        int stride_w = 1,
        int padding = 0
    ) {
        std::vector<Eigen::MatrixXf> outputs;
        for (const auto& kernel : kernels) {
            outputs.push_back(convolve2D(input, kernel, stride_h, stride_w, padding));
        }
        return outputs;
    }
};

#endif
