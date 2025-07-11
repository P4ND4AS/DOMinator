#include <vector>
#include <Eigen/Dense>

void applyReLU(std::vector<Eigen::MatrixXf>& featureMaps) {
    for (auto& map : featureMaps) {
        map = map.cwiseMax(0.0f);
    }
}