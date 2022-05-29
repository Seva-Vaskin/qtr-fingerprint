#include <gtest/gtest.h>

#include "Fingerprint.h"

template<std::size_t FINGERPRINT_SIZE>
void testNoAlign(std::size_t expectedCountOfBlocks) {
    Fingerprint<FINGERPRINT_SIZE> fingerprint;
    EXPECT_EQ(sizeof(fingerprint), expectedCountOfBlocks * sizeof(typename Fingerprint<FINGERPRINT_SIZE>::BlockType));
}

TEST(Fingerprint, NO_ALIGN) {
    testNoAlign<0>(0);
    testNoAlign<1>(1);
    testNoAlign<2>(1);
    testNoAlign<15>(1);
    testNoAlign<16>(1);
    testNoAlign<32>(1);
    testNoAlign<33>(1);
    testNoAlign<63>(1);
    testNoAlign<64>(1);
    testNoAlign<65>(2);
    testNoAlign<123>(2);
    testNoAlign<128>(2);
    testNoAlign<129>(3);
    testNoAlign<1024>(16);
    testNoAlign<10000>(157);
}


template<std::size_t FINGERPRINT_SIZE>
class FingerprintTest {
public:
    static void testBuildFromIndigo(std::vector<typename Fingerprint<FINGERPRINT_SIZE>::BlockType> expectedFingerprint,
                                    const char *fingerprint) {
        Fingerprint<FINGERPRINT_SIZE> f;
        f.buildFromIndigoFingerprint(fingerprint);
        EXPECT_EQ(expectedFingerprint.size(), Fingerprint<FINGERPRINT_SIZE>::countOfBlocks);
        for (int i = 0; i < expectedFingerprint.size(); ++i) {
            EXPECT_EQ(expectedFingerprint[i], f._data[i]);
        }
    }
};

TEST(buildFromIndigoFingerprint, BUILD_INDIGO_FINGERPRINT) {
    FingerprintTest<16 * 4>::testBuildFromIndigo({17842088918871736696ull}, "f79bd5571c488178");
    FingerprintTest<16 * 4>::testBuildFromIndigo({498755819845017657ull}, "06ebefc68f856039");
    FingerprintTest<32 * 4>::testBuildFromIndigo({16403606283573444974ull, 4489074196607866429ull},
                                                 "e3a551013fc2c16e3e4c65329b2f163d");
    FingerprintTest<32 * 4>::testBuildFromIndigo({15543249819049897699ull, 13414549369227920888ull},
                                                 "d7b4b75bd79faee3ba2a0d93f6c545f8");
    FingerprintTest<48 * 4>::testBuildFromIndigo(
            {7539348120054189930ull, 4683365180620864016ull, 934921229529721591ull},
            "68a1252b85d6cf6a40fea7d17856fa100cf981e7a3172af7");
    FingerprintTest<48 * 4>::testBuildFromIndigo(
            {1771275926324586302ull, 17682369433868156708ull, 15214827881143040204ull},
            "1894d61203c7273ef56465509cefbf24d325ed58a63d78cc");
}
