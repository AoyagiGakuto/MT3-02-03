#include <Novice.h>
#include <cmath>
#include <corecrt_math.h>
#include <cstdint>
#include <imgui.h>

const char kWindowTitle[] = "LE2D_02_アオヤギ_ガクト_確認課題";


#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

struct Segment {
    Vector3 start;
    Vector3 end;
};

struct Plane {
    Vector3 point; // 平面上の1点
    Vector3 normal; // 法線
};

// ベクトルのドット積
float Dot(const Vector3& a, const Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// ベクトル正規化
Vector3 Normalize(const Vector3& v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0001f) {
        return { v.x / len, v.y / len, v.z / len };
    }
    return v;
}

Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b)
{
    Matrix4x4 r {};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                r.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return r;
}

// ビュー＋プロジェクション行列
Matrix4x4 MakeViewProjectionMatrix(const Vector3& cameraTranslate, const Vector3& cameraRotate)
{
    float cosY = cosf(cameraRotate.y);
    float sinY = sinf(cameraRotate.y);
    float cosX = cosf(cameraRotate.x);
    float sinX = sinf(cameraRotate.x);
    float cosZ = cosf(cameraRotate.z);
    float sinZ = sinf(cameraRotate.z);

    Matrix4x4 rotY {};
    rotY.m[0][0] = cosY;
    rotY.m[0][2] = sinY;
    rotY.m[1][1] = 1.0f;
    rotY.m[2][0] = -sinY;
    rotY.m[2][2] = cosY;
    rotY.m[3][3] = 1.0f;

    Matrix4x4 rotX {};
    rotX.m[0][0] = 1.0f;
    rotX.m[1][1] = cosX;
    rotX.m[1][2] = -sinX;
    rotX.m[2][1] = sinX;
    rotX.m[2][2] = cosX;
    rotX.m[3][3] = 1.0f;

    Matrix4x4 rotZ {};
    rotZ.m[0][0] = cosZ;
    rotZ.m[0][1] = -sinZ;
    rotZ.m[1][0] = sinZ;
    rotZ.m[1][1] = cosZ;
    rotZ.m[2][2] = 1.0f;
    rotZ.m[3][3] = 1.0f;

    Matrix4x4 rot = Multiply(Multiply(rotZ, rotX), rotY);

    Matrix4x4 trans {};
    trans.m[0][0] = 1.0f;
    trans.m[1][1] = 1.0f;
    trans.m[2][2] = 1.0f;
    trans.m[3][3] = 1.0f;
    trans.m[3][0] = -cameraTranslate.x;
    trans.m[3][1] = -cameraTranslate.y;
    trans.m[3][2] = -cameraTranslate.z;

    Matrix4x4 view = Multiply(trans, rot);

    Matrix4x4 proj {};
    float fovY = 60.0f * (M_PI / 180.0f);
    float aspect = 1280.0f / 720.0f;
    float nearZ = 0.1f;
    float farZ = 100.0f;
    float f = 1.0f / tanf(fovY / 2.0f);

    proj.m[0][0] = f / aspect;
    proj.m[1][1] = f;
    proj.m[2][2] = farZ / (farZ - nearZ);
    proj.m[2][3] = (-nearZ * farZ) / (farZ - nearZ);
    proj.m[3][2] = 1.0f;
    proj.m[3][3] = 0.0f;

    return Multiply(view, proj);
}

// ビューポート変換行列
Matrix4x4 MakeViewportForMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
{
    Matrix4x4 m {};
    m.m[0][0] = width * 0.5f;
    m.m[1][1] = -height * 0.5f;
    m.m[2][2] = (maxDepth - minDepth);
    m.m[3][0] = left + width * 0.5f;
    m.m[3][1] = top + height * 0.5f;
    m.m[3][2] = minDepth;
    m.m[3][3] = 1.0f;
    return m;
}

// 座標変換
Vector3 Transform(const Vector3& v, const Matrix4x4& m)
{
    float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        x /= w;
        y /= w;
        z /= w;
    }
    return { x, y, z };
}

// グリッド描画
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix)
{
    const float kGridHalfWidth = 2.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

    // X方向（縦線）
    for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
        float x = -kGridHalfWidth + xIndex * kGridEvery;
        Vector3 start = { x, 0.0f, -kGridHalfWidth };
        Vector3 end = { x, 0.0f, kGridHalfWidth };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }

    // Z方向（横線）
    for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
        float z = -kGridHalfWidth + zIndex * kGridEvery;
        Vector3 start = { -kGridHalfWidth, 0.0f, z };
        Vector3 end = { kGridHalfWidth, 0.0f, z };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }
}

// 平面の簡易描画（XZ方向の四角形）
void DrawPlane(const Plane& plane, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color)
{
    float size = 2.0f;
    Vector3 corners[4] = {
        { plane.point.x - size, plane.point.y, plane.point.z - size },
        { plane.point.x + size, plane.point.y, plane.point.z - size },
        { plane.point.x + size, plane.point.y, plane.point.z + size },
        { plane.point.x - size, plane.point.y, plane.point.z + size }
    };

    for (int i = 0; i < 4; i++) {
        Vector3 p1 = Transform(corners[i], viewProjectionMatrix);
        p1 = Transform(p1, viewportMatrix);
        Vector3 p2 = Transform(corners[(i + 1) % 4], viewProjectionMatrix);
        p2 = Transform(p2, viewportMatrix);

        Novice::DrawLine((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, color);
    }
}

// 線分描画
void DrawSegment(const Segment& seg,
    const Matrix4x4& viewProjectionMatrix,
    const Matrix4x4& viewportMatrix,
    uint32_t color)
{

    Vector3 p1 = Transform(seg.start, viewProjectionMatrix);
    p1 = Transform(p1, viewportMatrix);
    Vector3 p2 = Transform(seg.end, viewProjectionMatrix);
    p2 = Transform(p2, viewportMatrix);

    Novice::DrawLine((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, color);
}

// 線分と平面の衝突判定
bool IsSegmentPlaneCollision(const Segment& seg, const Plane& plane, Vector3* outHitPoint = nullptr)
{
    Vector3 n = Normalize(plane.normal);

    Vector3 dir {
        seg.end.x - seg.start.x,
        seg.end.y - seg.start.y,
        seg.end.z - seg.start.z
    };

    float denom = Dot(n, dir);
    if (fabsf(denom) < 1e-6f) {
        return false; // 平行
    }

    float dist = Dot(n, { plane.point.x - seg.start.x, plane.point.y - seg.start.y, plane.point.z - seg.start.z });

    float t = dist / denom;

    if (t >= 0.0f && t <= 1.0f) {
        if (outHitPoint) {
            outHitPoint->x = seg.start.x + dir.x * t;
            outHitPoint->y = seg.start.y + dir.y * t;
            outHitPoint->z = seg.start.z + dir.z * t;
        }
        return true;
    }

    return false;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, 1280, 720);

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    // カメラ
    Vector3 cameraTranslate { 0.0f, -4.0f, -20.0f };
    Vector3 cameraRotate { -0.2f, 0.0f, 0.0f };

    // 線分初期値
    Segment seg = {
        { -1.0f, 2.0f, 0.0f }, // 始点
        { 1.0f, -0.3f, 0.0f } // 終点
    };

    // 平面初期値
    Plane plane;
    plane.point = { 0.0f, -0.5f, 0.0f };
    plane.normal = { 0.0f, 1.0f, 0.0f };

    Matrix4x4 viewportMatrix = MakeViewportForMatrix(
        0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f);

    Matrix4x4 viewProjectionMatrix = MakeViewProjectionMatrix(cameraTranslate, cameraRotate);

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓更新処理ここから
        ///

        viewProjectionMatrix = MakeViewProjectionMatrix(cameraTranslate, cameraRotate);

        // 衝突判定
        Vector3 hitPoint {};
        bool isHit = IsSegmentPlaneCollision(seg, plane, &hitPoint);
        
        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///
        
        // UI
        ImGui::Begin("Control");
        ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
        ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
        ImGui::DragFloat3("Seg Start", &seg.start.x, 0.01f);
        ImGui::DragFloat3("Seg End", &seg.end.x, 0.01f);
        ImGui::DragFloat3("Plane Point", &plane.point.x, 0.01f);
        ImGui::DragFloat3("Plane Normal", &plane.normal.x, 0.01f);
        ImGui::Text("Collision: %s", isHit ? "YES" : "NO");
        ImGui::End();

        // グリッド
        DrawGrid(viewProjectionMatrix, viewportMatrix);

        // 平面
        DrawPlane(plane, viewProjectionMatrix, viewportMatrix, BLUE);

        // 線分
        DrawSegment(seg, viewProjectionMatrix, viewportMatrix, isHit ? RED : WHITE);

        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}
