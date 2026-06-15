#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "我方飞机.h"
#include "敌方飞机.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")

using namespace Gdiplus;

namespace
{
    constexpr int kClientWidth = 540;
    constexpr int kClientHeight = 800;
    constexpr UINT_PTR kTimerId = 1;
    constexpr UINT kFrameMs = 16;
    constexpr int kDeathAnimationTicks = 63;
    constexpr int kPlayerLives = 5;
    constexpr int kEnemyLives = 2;

    enum class GameState
    {
        Cover,
        Menu,
        History,
        Playing,
        Dying,
        GameOver,
        Paused
    };

    enum class ButtonId
    {
        None,
        CoverStart,
        CoverMenu,
        MenuPlayerPrev,
        MenuPlayerNext,
        MenuBulletPrev,
        MenuBulletNext,
        MenuHistory,
        MenuStart,
        MenuBack,
        HistoryBack,
        GameOverStart,
        GameOverMenu
    };

    struct Layout
    {
        RECT coverStart{};
        RECT coverMenu{};
        RECT menuPlayerPrev{};
        RECT menuPlayerNext{};
        RECT menuBulletPrev{};
        RECT menuBulletNext{};
        RECT menuHistory{};
        RECT menuStart{};
        RECT menuBack{};
        RECT historyBack{};
        RECT gameOverStart{};
        RECT gameOverMenu{};
    };

    struct SpriteInfo
    {
        float width = 0.0f;
        float height = 0.0f;
    };

    struct Explosion
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 96.0f;
        float height = 96.0f;
        int age = 0;
        int duration = 24;
        bool active = true;
    };

    float ClampFloat(float value, float minValue, float maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }

    bool PointInRect(const RECT& rect, int x, int y)
    {
        return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
    }

    bool Intersects(const RECT& a, const RECT& b)
    {
        return !(a.right < b.left || a.left > b.right || a.bottom < b.top || a.top > b.bottom);
    }

    RECT MakeRect(int left, int top, int right, int bottom)
    {
        return RECT{ left, top, right, bottom };
    }

    std::wstring ExeDir()
    {
        wchar_t buffer[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        std::wstring path = buffer;
        const size_t slash = path.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
        {
            path = path.substr(0, slash + 1);
        }
        return path;
    }

    std::wstring AssetPath(const std::wstring& fileName)
    {
        return ExeDir() + L"assets\\" + fileName;
    }

    std::wstring HistoryPath()
    {
        return ExeDir() + L"history_scores.txt";
    }

    Bitmap* LoadBitmapSafe(const std::wstring& path)
    {
        Bitmap* bitmap = Bitmap::FromFile(path.c_str(), FALSE);
        if (bitmap == nullptr || bitmap->GetLastStatus() != Ok)
        {
            delete bitmap;
            return nullptr;
        }
        return bitmap;
    }

    void DrawBitmapScaled(Graphics& graphics, Bitmap* bitmap, float x, float y, float width, float height)
    {
        if (bitmap != nullptr)
        {
            graphics.DrawImage(bitmap, x, y, width, height);
        }
    }

    void DrawTextBlock(Graphics& graphics, const std::wstring& text, Font& font, Brush& brush, const RectF& rect, StringAlignment align = StringAlignmentCenter)
    {
        StringFormat format;
        format.SetAlignment(align);
        format.SetLineAlignment(StringAlignmentCenter);
        graphics.DrawString(text.c_str(), -1, &font, rect, &format, &brush);
    }

    struct Assets
    {
        Bitmap* cover = nullptr;
        Bitmap* background = nullptr;
        Bitmap* explosion = nullptr;
        Bitmap* player[3] = {};
        Bitmap* enemy[3] = {};
        Bitmap* playerBullet[3] = {};
        Bitmap* enemyBullet[2] = {};

        ~Assets()
        {
            delete cover;
            delete background;
            delete explosion;
            for (Bitmap*& bitmap : player) delete bitmap;
            for (Bitmap*& bitmap : enemy) delete bitmap;
            for (Bitmap*& bitmap : playerBullet) delete bitmap;
            for (Bitmap*& bitmap : enemyBullet) delete bitmap;
        }
    };

    class GameApp
    {
    public:
        bool Init(HWND hwnd)
        {
            hwnd_ = hwnd;
            rng_.seed(static_cast<unsigned int>(GetTickCount()));

            assets_.cover = LoadBitmapSafe(AssetPath(L"封面.jpg"));
            assets_.background = LoadBitmapSafe(AssetPath(L"背景.png"));
            assets_.explosion = LoadBitmapSafe(AssetPath(L"爆炸.png"));
            assets_.player[2] = LoadBitmapSafe(AssetPath(L"我方飞机3.PNG"));
            assets_.enemy[0] = LoadBitmapSafe(AssetPath(L"敌方飞机1.PNG"));
            assets_.enemy[1] = LoadBitmapSafe(AssetPath(L"敌方飞机2.PNG"));
            assets_.enemy[2] = LoadBitmapSafe(AssetPath(L"敌方飞机3.PNG"));
            assets_.playerBullet[0] = LoadBitmapSafe(AssetPath(L"子弹1.PNG"));
            assets_.playerBullet[1] = LoadBitmapSafe(AssetPath(L"子弹2.PNG"));
            assets_.playerBullet[2] = LoadBitmapSafe(AssetPath(L"子弹3.PNG"));
            assets_.enemyBullet[0] = LoadBitmapSafe(AssetPath(L"敌方子弹1.PNG"));
            assets_.enemyBullet[1] = LoadBitmapSafe(AssetPath(L"敌方子弹2.PNG"));

            for (int i = 0; i < 3; ++i)
            {
                if (assets_.player[i] != nullptr)
                {
                    playerSprites_[i].width = static_cast<float>(assets_.player[i]->GetWidth());
                    playerSprites_[i].height = static_cast<float>(assets_.player[i]->GetHeight());
                }
                if (assets_.playerBullet[i] != nullptr)
                {
                    playerBullets_[i].width = static_cast<float>(assets_.playerBullet[i]->GetWidth());
                    playerBullets_[i].height = static_cast<float>(assets_.playerBullet[i]->GetHeight());
                }
                if (assets_.enemy[i] != nullptr)
                {
                    enemySprite_.width = static_cast<float>(assets_.enemy[i]->GetWidth());
                    enemySprite_.height = static_cast<float>(assets_.enemy[i]->GetHeight());
                }
            }
            for (int i = 0; i < 2; ++i)
            {
                if (assets_.enemyBullet[i] != nullptr)
                {
                    enemyBullets_[i].width = static_cast<float>(assets_.enemyBullet[i]->GetWidth());
                    enemyBullets_[i].height = static_cast<float>(assets_.enemyBullet[i]->GetHeight());
                }
            }
            if (assets_.explosion != nullptr)
            {
                explosionSprite_.width = static_cast<float>(assets_.explosion->GetWidth());
                explosionSprite_.height = static_cast<float>(assets_.explosion->GetHeight());
            }

            playerSkin_ = 2;

            LoadHistoryScores();
            ResetToCover();
            return true;
        }

        void ResetToCover()
        {
            state_ = GameState::Cover;
            score_ = 0;
            spawnCounter_ = 30;
            backgroundOffset_ = 0.0f;
            frameTick_ = 0;
            dyingTicks_ = 0;
            hoverButton_ = ButtonId::None;
            enemies_.clear();
            bullets_.clear();
            explosions_.clear();
            message_.clear();
            playerDeadSaved_ = false;
            player_.Reset(kClientWidth * 0.5f, kClientHeight - 120.0f);
            player_.SetSize(PlayerWidth(), PlayerHeight());
        }

        void OpenMenu()
        {
            state_ = GameState::Menu;
            hoverButton_ = ButtonId::None;
        }

        void OpenHistory()
        {
            state_ = GameState::History;
            hoverButton_ = ButtonId::None;
        }

        void StartGame()
        {
            state_ = GameState::Playing;
            score_ = 0;
            spawnCounter_ = 20;
            backgroundOffset_ = 0.0f;
            frameTick_ = 0;
            dyingTicks_ = 0;
            message_.clear();
            enemies_.clear();
            bullets_.clear();
            explosions_.clear();
            playerDeadSaved_ = false;
            player_.Reset(kClientWidth * 0.5f, kClientHeight - 120.0f);
            player_.SetSize(PlayerWidth(), PlayerHeight());
        }

        void OnKeyDown(WPARAM key)
        {
            if (key == VK_ESCAPE)
            {
                DestroyWindow(hwnd_);
                return;
            }

            switch (state_)
            {
            case GameState::Cover:
                if (key == VK_RETURN)
                {
                    StartGame();
                }
                else if (key == 'M')
                {
                    OpenMenu();
                }
                break;
            case GameState::Menu:
                if (key == VK_RETURN)
                {
                    StartGame();
                }
                else if (key == VK_LEFT)
                {
                    CyclePlayerSkin(-1);
                }
                else if (key == VK_RIGHT)
                {
                    CyclePlayerSkin(1);
                }
                else if (key == VK_UP)
                {
                    CycleBulletSkin(-1);
                }
                else if (key == VK_DOWN)
                {
                    CycleBulletSkin(1);
                }
                else if (key == 'H')
                {
                    OpenHistory();
                }
                else if (key == 'B')
                {
                    ResetToCover();
                }
                break;
            case GameState::History:
                if (key == VK_RETURN || key == 'B')
                {
                    OpenMenu();
                }
                break;
            case GameState::Paused:
                if (key == 'P')
                {
                    state_ = GameState::Playing;
                }
                else if (key == 'R')
                {
                    StartGame();
                }
                else if (key == 'M')
                {
                    ResetToCover();
                }
                break;
            case GameState::GameOver:
                if (key == VK_RETURN)
                {
                    StartGame();
                }
                else if (key == 'M')
                {
                    OpenMenu();
                }
                break;
            default:
                break;
            }
        }

        void OnMouseMove(int x, int y)
        {
            const ButtonId next = HitTestButton(x, y);
            if (next != hoverButton_)
            {
                hoverButton_ = next;
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
        }

        void OnLButtonDown(int x, int y)
        {
            const ButtonId button = HitTestButton(x, y);
            switch (state_)
            {
            case GameState::Cover:
                if (button == ButtonId::CoverStart) StartGame();
                else if (button == ButtonId::CoverMenu) OpenMenu();
                break;
            case GameState::Menu:
                if (button == ButtonId::MenuPlayerPrev) CyclePlayerSkin(-1);
                else if (button == ButtonId::MenuPlayerNext) CyclePlayerSkin(1);
                else if (button == ButtonId::MenuBulletPrev) CycleBulletSkin(-1);
                else if (button == ButtonId::MenuBulletNext) CycleBulletSkin(1);
                else if (button == ButtonId::MenuHistory) OpenHistory();
                else if (button == ButtonId::MenuStart) StartGame();
                else if (button == ButtonId::MenuBack) ResetToCover();
                break;
            case GameState::History:
                if (button == ButtonId::HistoryBack) OpenMenu();
                break;
            case GameState::GameOver:
                if (button == ButtonId::GameOverStart) StartGame();
                else if (button == ButtonId::GameOverMenu) OpenMenu();
                break;
            default:
                break;
            }
        }

        void Update()
        {
            ++frameTick_;
            UpdateExplosions();

            if (state_ == GameState::Cover || state_ == GameState::Menu || state_ == GameState::History || state_ == GameState::Paused || state_ == GameState::GameOver)
            {
                return;
            }

            if (state_ == GameState::Dying)
            {
                if (--dyingTicks_ <= 0)
                {
                    ResetToCover();
                }
                return;
            }

            backgroundOffset_ += 1.4f;
            if (backgroundOffset_ >= static_cast<float>(kClientHeight))
            {
                backgroundOffset_ = 0.0f;
            }

            UpdatePlayer();
            UpdateEnemies();
            UpdateBullets();
            HandleCollisions();
            SpawnEnemies();

            if (player_.Lives() <= 0)
            {
                TriggerPlayerDeath();
            }
        }

        void Render(HDC hdc)
        {
            RECT clientRect{};
            GetClientRect(hwnd_, &clientRect);
            const int width = std::max(1L, clientRect.right - clientRect.left);
            const int height = std::max(1L, clientRect.bottom - clientRect.top);

            HDC memDc = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(memDc, memBmp));

            Graphics graphics(memDc);
            graphics.SetSmoothingMode(SmoothingModeHighQuality);
            graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            layout_ = BuildLayout(static_cast<float>(width), static_cast<float>(height));

            switch (state_)
            {
            case GameState::Cover:
                DrawCover(graphics, static_cast<float>(width), static_cast<float>(height));
                break;
            case GameState::Menu:
                DrawMenu(graphics, static_cast<float>(width), static_cast<float>(height));
                break;
            case GameState::History:
                DrawHistory(graphics, static_cast<float>(width), static_cast<float>(height));
                break;
            case GameState::Playing:
            case GameState::Paused:
            case GameState::Dying:
            case GameState::GameOver:
                DrawGameplay(graphics, static_cast<float>(width), static_cast<float>(height));
                if (state_ == GameState::Paused)
                {
                    DrawPausedOverlay(graphics, static_cast<float>(width), static_cast<float>(height));
                }
                if (state_ == GameState::Dying)
                {
                    DrawDyingOverlay(graphics, static_cast<float>(width), static_cast<float>(height));
                }
                if (state_ == GameState::GameOver)
                {
                    DrawGameOver(graphics, static_cast<float>(width), static_cast<float>(height));
                }
                break;
            }

            BitBlt(hdc, 0, 0, width, height, memDc, 0, 0, SRCCOPY);
            SelectObject(memDc, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDc);
        }

    private:
        void DrawBackground(Graphics& graphics, float width, float height)
        {
            if (assets_.background != nullptr && assets_.background->GetWidth() > 0 && assets_.background->GetHeight() > 0)
            {
                const float imageWidth = static_cast<float>(assets_.background->GetWidth());
                const float imageHeight = static_cast<float>(assets_.background->GetHeight());
                const float scale = std::max(width / imageWidth, height / imageHeight);
                const float drawWidth = imageWidth * scale;
                const float drawHeight = imageHeight * scale;
                const float offsetY = std::fmod(backgroundOffset_, drawHeight);
                graphics.DrawImage(assets_.background, 0.0f, -offsetY, drawWidth, drawHeight);
                graphics.DrawImage(assets_.background, 0.0f, -offsetY + drawHeight, drawWidth, drawHeight);
            }
            else
            {
                graphics.Clear(Color(255, 10, 12, 24));
            }
            SolidBrush wash(Color(90, 0, 0, 0));
            graphics.FillRectangle(&wash, 0.0f, 0.0f, width, height);
        }

        void DrawCoverBackdrop(Graphics& graphics, float width, float height)
        {
            Bitmap* backdrop = assets_.cover != nullptr ? assets_.cover : assets_.background;
            if (backdrop != nullptr && backdrop->GetWidth() > 0 && backdrop->GetHeight() > 0)
            {
                const float imageWidth = static_cast<float>(backdrop->GetWidth());
                const float imageHeight = static_cast<float>(backdrop->GetHeight());
                const float scale = std::max(width / imageWidth, height / imageHeight);
                const float drawWidth = imageWidth * scale;
                const float drawHeight = imageHeight * scale;
                graphics.DrawImage(backdrop, 0.0f, 0.0f, drawWidth, drawHeight);
            }
            else
            {
                graphics.Clear(Color(255, 10, 12, 24));
            }

            SolidBrush wash(Color(115, 0, 0, 0));
            graphics.FillRectangle(&wash, 0.0f, 0.0f, width, height);
        }

        void DrawButton(Graphics& graphics, const RECT& rect, const std::wstring& text, bool active, float fontSize = 18.0f)
        {
            FontFamily fontFamily(L"Microsoft YaHei");
            Font font(&fontFamily, fontSize, FontStyleBold, UnitPixel);
            SolidBrush fill(active ? Color(200, 70, 120, 255) : Color(170, 10, 18, 40));
            SolidBrush textBrush(Color(255, 255, 255, 255));
            Pen border(active ? Color(255, 230, 240, 255) : Color(170, 150, 180, 255), 2.0f);
            graphics.FillRectangle(&fill, static_cast<REAL>(rect.left), static_cast<REAL>(rect.top), static_cast<REAL>(rect.right - rect.left), static_cast<REAL>(rect.bottom - rect.top));
            graphics.DrawRectangle(&border, static_cast<REAL>(rect.left), static_cast<REAL>(rect.top), static_cast<REAL>(rect.right - rect.left), static_cast<REAL>(rect.bottom - rect.top));
            DrawTextBlock(graphics, text, font, textBrush, RectF(static_cast<REAL>(rect.left), static_cast<REAL>(rect.top), static_cast<REAL>(rect.right - rect.left), static_cast<REAL>(rect.bottom - rect.top)));
        }

        void DrawSelectionPreview(Graphics& graphics, float x, float y, float width, float height, Bitmap* bitmap)
        {
            SolidBrush panel(Color(120, 12, 14, 28));
            Pen border(Color(170, 140, 180, 255), 1.0f);
            graphics.FillRectangle(&panel, x, y, width, height);
            graphics.DrawRectangle(&border, x, y, width, height);
            if (bitmap != nullptr)
            {
                const float imageWidth = static_cast<float>(bitmap->GetWidth());
                const float imageHeight = static_cast<float>(bitmap->GetHeight());
                if (imageWidth > 0.0f && imageHeight > 0.0f)
                {
                    const float scale = std::min(width / imageWidth, height / imageHeight);
                    const float drawWidth = imageWidth * scale;
                    const float drawHeight = imageHeight * scale;
                    const float drawX = x + (width - drawWidth) * 0.5f;
                    const float drawY = y + (height - drawHeight) * 0.5f;
                    graphics.DrawImage(bitmap, drawX, drawY, drawWidth, drawHeight);
                }
            }
        }

        void DrawCover(Graphics& graphics, float width, float height)
        {
            DrawCoverBackdrop(graphics, width, height);

            FontFamily fontFamily(L"Microsoft YaHei");
            Font titleFont(&fontFamily, 30.0f, FontStyleBold, UnitPixel);
            Font promptFont(&fontFamily, 16.0f, FontStyleRegular, UnitPixel);
            SolidBrush white(Color(255, 255, 255, 255));
            SolidBrush pale(Color(230, 225, 236, 255));

            DrawTextBlock(graphics, L"空战突击", titleFont, white, RectF(0.0f, height * 0.20f, width, 48.0f));
            DrawTextBlock(graphics, L"回车开始    M 进入菜单", promptFont, pale, RectF(0.0f, height * 0.30f, width, 28.0f));

            DrawButton(graphics, layout_.coverStart, L"", hoverButton_ == ButtonId::CoverStart, 20.0f);
            DrawButton(graphics, layout_.coverMenu, L"", hoverButton_ == ButtonId::CoverMenu, 20.0f);
        }

        void DrawMenu(Graphics& graphics, float width, float height)
        {
            DrawCoverBackdrop(graphics, width, height);
            SolidBrush panel(Color(170, 8, 12, 28));
            Pen border(Color(180, 120, 180, 255), 2.0f);
            graphics.FillRectangle(&panel, 24.0f, 60.0f, width - 48.0f, height - 120.0f);
            graphics.DrawRectangle(&border, 24.0f, 60.0f, width - 48.0f, height - 120.0f);
            DrawSelectionPreview(graphics, 40.0f, 160.0f, 110.0f, 90.0f, assets_.player[playerSkin_]);
            DrawSelectionPreview(graphics, 40.0f, 304.0f, 110.0f, 90.0f, assets_.playerBullet[bulletSkin_]);
            DrawButton(graphics, layout_.menuPlayerPrev, L"", hoverButton_ == ButtonId::MenuPlayerPrev, 14.0f);
            DrawButton(graphics, layout_.menuPlayerNext, L"", hoverButton_ == ButtonId::MenuPlayerNext, 14.0f);
            DrawButton(graphics, layout_.menuBulletPrev, L"", hoverButton_ == ButtonId::MenuBulletPrev, 14.0f);
            DrawButton(graphics, layout_.menuBulletNext, L"", hoverButton_ == ButtonId::MenuBulletNext, 14.0f);
            DrawButton(graphics, layout_.menuHistory, L"", hoverButton_ == ButtonId::MenuHistory, 18.0f);
            DrawButton(graphics, layout_.menuStart, L"", hoverButton_ == ButtonId::MenuStart, 18.0f);
            DrawButton(graphics, layout_.menuBack, L"", hoverButton_ == ButtonId::MenuBack, 18.0f);
        }

        void DrawHistory(Graphics& graphics, float width, float height)
        {
            DrawBackground(graphics, width, height);
            SolidBrush panel(Color(170, 8, 12, 28));
            Pen border(Color(180, 120, 180, 255), 2.0f);
            graphics.FillRectangle(&panel, 24.0f, 60.0f, width - 48.0f, height - 120.0f);
            graphics.DrawRectangle(&border, 24.0f, 60.0f, width - 48.0f, height - 120.0f);

            std::vector<int> scores = historyScores_;
            std::sort(scores.begin(), scores.end(), std::greater<int>());
            if (!scores.empty())
            {
                const int maxShow = std::min<int>(10, scores.size());
                FontFamily fontFamily(L"Microsoft YaHei");
                Font itemFont(&fontFamily, 15.0f, FontStyleRegular, UnitPixel);
                SolidBrush pale(Color(230, 225, 236, 255));
                for (int i = 0; i < maxShow; ++i)
                {
                    std::wstring line = std::to_wstring(i + 1) + L". " + std::to_wstring(scores[i]);
                    DrawTextBlock(graphics, line, itemFont, pale, RectF(40.0f, 150.0f + i * 34.0f, width - 80.0f, 22.0f), StringAlignmentNear);
                }
            }

            DrawButton(graphics, layout_.historyBack, L"", hoverButton_ == ButtonId::HistoryBack, 18.0f);
        }

        void DrawGameplay(Graphics& graphics, float width, float height)
        {
            DrawBackground(graphics, width, height);
            DrawExplosions(graphics);
            DrawBullets(graphics);
            DrawEnemies(graphics);
            DrawPlayer(graphics);
            DrawHud(graphics);
        }

        void DrawHud(Graphics& graphics)
        {
            SolidBrush panel(Color(165, 8, 12, 28));
            Pen border(Color(180, 120, 180, 255), 1.5f);
            graphics.FillRectangle(&panel, 14.0f, 12.0f, 190.0f, 68.0f);
            graphics.DrawRectangle(&border, 14.0f, 12.0f, 190.0f, 68.0f);

            FontFamily fontFamily(L"Microsoft YaHei");
            Font scoreFont(&fontFamily, 15.0f, FontStyleBold, UnitPixel);
            SolidBrush white(Color(255, 255, 255, 255));

            DrawTextBlock(graphics, std::to_wstring(score_), scoreFont, white, RectF(24.0f, 18.0f, 100.0f, 22.0f), StringAlignmentNear);

            const float barX = 60.0f;
            const float barY = 44.0f;
            const float barW = 122.0f;
            const float gap = 3.0f;
            const float segmentW = (barW - gap * 4.0f) / 5.0f;
            for (int i = 0; i < kPlayerLives; ++i)
            {
                SolidBrush segment(i < player_.Lives() ? Color(255, 240, 80, 80) : Color(120, 60, 60, 60));
                graphics.FillRectangle(&segment, barX + i * (segmentW + gap), barY, segmentW, 12.0f);
            }
        }

        void DrawGameOver(Graphics& graphics, float width, float height)
        {
            SolidBrush panel(Color(170, 0, 0, 0));
            graphics.FillRectangle(&panel, 0.0f, 0.0f, width, height);
            DrawButton(graphics, layout_.gameOverStart, L"", hoverButton_ == ButtonId::GameOverStart, 18.0f);
            DrawButton(graphics, layout_.gameOverMenu, L"", hoverButton_ == ButtonId::GameOverMenu, 18.0f);
        }

        void DrawPausedOverlay(Graphics& graphics, float width, float height)
        {
            SolidBrush panel(Color(150, 0, 0, 0));
            graphics.FillRectangle(&panel, 0.0f, 0.0f, width, height);
        }

        void DrawDyingOverlay(Graphics& graphics, float width, float height)
        {
            SolidBrush panel(Color(110, 0, 0, 0));
            graphics.FillRectangle(&panel, 0.0f, 0.0f, width, height);
        }

        void DrawPlayer(Graphics& graphics)
        {
            Bitmap* bitmap = assets_.player[2];
            const float width = PlayerWidth();
            const float height = PlayerHeight();
            if (bitmap != nullptr)
            {
                DrawBitmapScaled(graphics, bitmap, player_.X(), player_.Y(), width, height);
            }
            else
            {
                SolidBrush brush(Color(255, 70, 180, 255));
                graphics.FillEllipse(&brush, player_.X(), player_.Y(), width, height);
            }

        }

        void DrawEnemies(Graphics& graphics)
        {
            for (const EnemyPlane& enemy : enemies_)
            {
                if (enemy.IsDead())
                {
                    continue;
                }
                Bitmap* bitmap = assets_.enemy[enemy.Kind() % 3];
                if (bitmap != nullptr)
                {
                    DrawBitmapScaled(graphics, bitmap, enemy.X(), enemy.Y(), enemy.Width(), enemy.Height());
                }
                else
                {
                    SolidBrush brush(Color(255, 255, 110, 90));
                    graphics.FillEllipse(&brush, enemy.X(), enemy.Y(), enemy.Width(), enemy.Height());
                }
            }
        }

        void DrawBullets(Graphics& graphics)
        {
            for (const Bullet& bullet : bullets_)
            {
                if (!bullet.active)
                {
                    continue;
                }
                Bitmap* bitmap = bullet.fromPlayer ? assets_.playerBullet[bullet.spriteIndex % 3] : assets_.enemyBullet[bullet.spriteIndex % 2];
                if (bitmap != nullptr)
                {
                    DrawBitmapScaled(graphics, bitmap, bullet.x, bullet.y, bullet.width, bullet.height);
                }
                else
                {
                    SolidBrush brush(bullet.fromPlayer ? Color(255, 80, 220, 255) : Color(255, 255, 90, 90));
                    graphics.FillEllipse(&brush, bullet.x, bullet.y, bullet.width, bullet.height);
                }
            }
        }

        void DrawExplosions(Graphics& graphics)
        {
            for (const Explosion& explosion : explosions_)
            {
                if (!explosion.active)
                {
                    continue;
                }
                const float progress = static_cast<float>(explosion.age) / static_cast<float>(std::max(1, explosion.duration));
                const float scale = 0.55f + progress * 1.1f;
                const float drawW = explosion.width * scale;
                const float drawH = explosion.height * scale;
                const float x = explosion.x + (explosion.width - drawW) * 0.5f;
                const float y = explosion.y + (explosion.height - drawH) * 0.5f;
                if (assets_.explosion != nullptr)
                {
                    graphics.DrawImage(assets_.explosion, x, y, drawW, drawH);
                }
                else
                {
                    SolidBrush brush(Color(220, 255, 180, 80));
                    graphics.FillEllipse(&brush, x, y, drawW, drawH);
                }
            }
        }

        void UpdatePlayer()
        {
            const bool left = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0 || (GetAsyncKeyState('A') & 0x8000) != 0;
            const bool right = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0 || (GetAsyncKeyState('D') & 0x8000) != 0;
            const bool up = (GetAsyncKeyState(VK_UP) & 0x8000) != 0 || (GetAsyncKeyState('W') & 0x8000) != 0;
            const bool down = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0 || (GetAsyncKeyState('S') & 0x8000) != 0;
            const bool shoot = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

            float dx = 0.0f;
            float dy = 0.0f;
            constexpr float speed = 6.0f;
            if (left) dx -= speed;
            if (right) dx += speed;
            if (up) dy -= speed;
            if (down) dy += speed;

            player_.Move(dx, dy, 12.0f, 80.0f, kClientWidth - PlayerWidth() - 12.0f, kClientHeight - PlayerHeight() - 12.0f);
            player_.Tick();

            if (shoot && player_.CanShoot())
            {
                Bullet bullet = player_.Shoot();
                ApplyPlayerBulletSize(bullet);
                bullets_.push_back(bullet);
            }
        }

        void UpdateEnemies()
        {
            for (EnemyPlane& enemy : enemies_)
            {
                if (enemy.IsDead())
                {
                    enemy.Tick();
                    continue;
                }
                enemy.Move(0.0f, 0.0f);
                enemy.Tick();
                if (enemy.CanShoot())
                {
                    Bullet bullet = enemy.Shoot();
                    ApplyEnemyBulletSize(bullet);
                    bullets_.push_back(bullet);
                }
            }

            for (EnemyPlane& enemy : enemies_)
            {
                if (!enemy.IsDead() && enemy.Y() > static_cast<float>(kClientHeight - 58))
                {
                    DamagePlayer();
                    enemy.Hit();
                    enemy.Hit();
                    HandleEnemyDeath(enemy);
                }
            }

            enemies_.erase(
                std::remove_if(enemies_.begin(), enemies_.end(), [](const EnemyPlane& enemy)
                {
                    return enemy.IsExpired() || enemy.Y() > static_cast<float>(kClientHeight + 90);
                }),
                enemies_.end());
        }

        void UpdateBullets()
        {
            for (Bullet& bullet : bullets_)
            {
                if (!bullet.active) continue;
                bullet.x += bullet.speedX;
                bullet.y += bullet.speedY;
                if (bullet.x < -50.0f || bullet.x > kClientWidth + 50.0f || bullet.y < -50.0f || bullet.y > kClientHeight + 50.0f)
                {
                    bullet.active = false;
                }
            }

            bullets_.erase(
                std::remove_if(bullets_.begin(), bullets_.end(), [](const Bullet& bullet)
                {
                    return !bullet.active;
                }),
                bullets_.end());
        }

        void HandleCollisions()
        {
            const RECT playerRect = player_.Bounds();
            for (Bullet& bullet : bullets_)
            {
                if (!bullet.active) continue;
                const RECT bulletRect = MakeRect(
                    static_cast<int>(bullet.x),
                    static_cast<int>(bullet.y),
                    static_cast<int>(bullet.x + bullet.width),
                    static_cast<int>(bullet.y + bullet.height));

                if (bullet.fromPlayer)
                {
                    for (EnemyPlane& enemy : enemies_)
                    {
                        if (enemy.IsDead()) continue;
                        if (Intersects(bulletRect, enemy.Bounds()))
                        {
                            bullet.active = false;
                            enemy.Hit();
                            HandleEnemyDeath(enemy);
                            break;
                        }
                    }
                }
                else if (!player_.IsInvincible() && Intersects(bulletRect, playerRect))
                {
                    bullet.active = false;
                    DamagePlayer();
                }
            }

            for (EnemyPlane& enemy : enemies_)
            {
                if (!enemy.IsDead() && !player_.IsInvincible() && Intersects(playerRect, enemy.Bounds()))
                {
                    enemy.Hit();
                    enemy.Hit();
                    DamagePlayer();
                    HandleEnemyDeath(enemy);
                }
            }
        }

        void SpawnEnemies()
        {
            --spawnCounter_;
            if (spawnCounter_ > 0)
            {
                return;
            }

            std::uniform_int_distribution<int> countDist(1, 3);
            std::uniform_real_distribution<float> driftDist(-1.0f, 1.0f);
            const float enemyWidth = enemySprite_.width > 0.0f ? enemySprite_.width : 68.0f;
            std::uniform_real_distribution<float> xDist(18.0f, kClientWidth - enemyWidth - 18.0f);

            const int count = countDist(rng_);
            for (int i = 0; i < count; ++i)
            {
                EnemyPlane enemy;
                enemy.SetSize(enemyWidth, enemySprite_.height > 0.0f ? enemySprite_.height : 68.0f);
                const float speedY = 2.0f + static_cast<float>(std::min(score_, 250)) * 0.01f + static_cast<float>(i) * 0.2f;
                enemy.Reset(xDist(rng_), -50.0f - static_cast<float>(i * 36), i % 3, driftDist(rng_), speedY, kEnemyLives);
                enemies_.push_back(enemy);
            }

            spawnCounter_ = std::max(16, 36 - score_ / 18);
        }

        void UpdateExplosions()
        {
            for (Explosion& explosion : explosions_)
            {
                if (!explosion.active) continue;
                ++explosion.age;
                if (explosion.age >= explosion.duration)
                {
                    explosion.active = false;
                }
            }

            explosions_.erase(
                std::remove_if(explosions_.begin(), explosions_.end(), [](const Explosion& explosion)
                {
                    return !explosion.active;
                }),
                explosions_.end());
        }

        void AddExplosion(float x, float y, float width, float height, int duration)
        {
            Explosion explosion;
            explosion.x = x;
            explosion.y = y;
            explosion.width = width;
            explosion.height = height;
            explosion.duration = duration;
            explosions_.push_back(explosion);
        }

        void DamagePlayer()
        {
            player_.Hit();
            if (player_.Lives() <= 0 && state_ == GameState::Playing)
            {
                TriggerPlayerDeath();
            }
        }

        void TriggerPlayerDeath()
        {
            AddExplosion(player_.X() - 30.0f, player_.Y() - 30.0f, PlayerWidth() + 60.0f, PlayerHeight() + 60.0f, kDeathAnimationTicks);
            enemies_.clear();
            bullets_.clear();
            state_ = GameState::Dying;
            dyingTicks_ = kDeathAnimationTicks;
            if (!playerDeadSaved_)
            {
                SaveScore(score_);
                playerDeadSaved_ = true;
            }
        }

        void HandleEnemyDeath(EnemyPlane& enemy)
        {
            if (!enemy.IsDead() || enemy.DeathTicks() > 0)
            {
                return;
            }

            enemy.BeginDeath(kDeathAnimationTicks);
            score_ += 10;
            AddExplosion(enemy.X(), enemy.Y(), enemy.Width(), enemy.Height(), kDeathAnimationTicks);
        }

        void SaveScore(int score)
        {
            historyScores_.push_back(score);
            std::wofstream out;
            out.open(HistoryPath(), std::ios::app);
            if (out.is_open())
            {
                out << score << L"\n";
            }
        }

        void LoadHistoryScores()
        {
            historyScores_.clear();
            std::wifstream in;
            in.open(HistoryPath());
            if (!in.is_open())
            {
                return;
            }
            int value = 0;
            while (in >> value)
            {
                historyScores_.push_back(value);
            }
        }

        ButtonId HitTestButton(int x, int y) const
        {
            switch (state_)
            {
            case GameState::Cover:
                if (PointInRect(layout_.coverStart, x, y)) return ButtonId::CoverStart;
                if (PointInRect(layout_.coverMenu, x, y)) return ButtonId::CoverMenu;
                break;
            case GameState::Menu:
                if (PointInRect(layout_.menuPlayerPrev, x, y)) return ButtonId::MenuPlayerPrev;
                if (PointInRect(layout_.menuPlayerNext, x, y)) return ButtonId::MenuPlayerNext;
                if (PointInRect(layout_.menuBulletPrev, x, y)) return ButtonId::MenuBulletPrev;
                if (PointInRect(layout_.menuBulletNext, x, y)) return ButtonId::MenuBulletNext;
                if (PointInRect(layout_.menuHistory, x, y)) return ButtonId::MenuHistory;
                if (PointInRect(layout_.menuStart, x, y)) return ButtonId::MenuStart;
                if (PointInRect(layout_.menuBack, x, y)) return ButtonId::MenuBack;
                break;
            case GameState::History:
                if (PointInRect(layout_.historyBack, x, y)) return ButtonId::HistoryBack;
                break;
            case GameState::GameOver:
                if (PointInRect(layout_.gameOverStart, x, y)) return ButtonId::GameOverStart;
                if (PointInRect(layout_.gameOverMenu, x, y)) return ButtonId::GameOverMenu;
                break;
            default:
                break;
            }
            return ButtonId::None;
        }

        Layout BuildLayout(float width, float height) const
        {
            Layout layout;
            layout.coverStart = MakeRect(static_cast<int>(width * 0.34f), static_cast<int>(height * 0.48f), static_cast<int>(width * 0.66f), static_cast<int>(height * 0.56f));
            layout.coverMenu = MakeRect(static_cast<int>(width * 0.34f), static_cast<int>(height * 0.59f), static_cast<int>(width * 0.66f), static_cast<int>(height * 0.67f));
            layout.menuPlayerPrev = MakeRect(160, 160, 220, 194);
            layout.menuPlayerNext = MakeRect(320, 160, 380, 194);
            layout.menuBulletPrev = MakeRect(160, 308, 220, 342);
            layout.menuBulletNext = MakeRect(320, 308, 380, 342);
            layout.menuHistory = MakeRect(60, 460, 240, 504);
            layout.menuStart = MakeRect(300, 460, 480, 504);
            layout.menuBack = MakeRect(180, 526, 360, 570);
            layout.historyBack = MakeRect(180, 650, 360, 694);
            layout.gameOverStart = MakeRect(160, 500, 380, 544);
            layout.gameOverMenu = MakeRect(160, 562, 380, 606);
            return layout;
        }

        void CyclePlayerSkin(int delta)
        {
            playerSkin_ = 2;
        }

        void CycleBulletSkin(int delta)
        {
            bulletSkin_ = (bulletSkin_ + delta + 3) % 3;
        }

        float PlayerWidth() const
        {
            return playerSprites_[2].width > 0.0f ? playerSprites_[2].width : 72.0f;
        }

        float PlayerHeight() const
        {
            return playerSprites_[2].height > 0.0f ? playerSprites_[2].height : 72.0f;
        }

        void ApplyPlayerBulletSize(Bullet& bullet) const
        {
            bullet.width = playerBullets_[bulletSkin_].width > 0.0f ? playerBullets_[bulletSkin_].width : 14.0f;
            bullet.height = playerBullets_[bulletSkin_].height > 0.0f ? playerBullets_[bulletSkin_].height : 28.0f;
        }

        void ApplyEnemyBulletSize(Bullet& bullet) const
        {
            bullet.width = enemyBullets_[0].width > 0.0f ? enemyBullets_[0].width : 14.0f;
            bullet.height = enemyBullets_[0].height > 0.0f ? enemyBullets_[0].height : 24.0f;
        }

    private:
        HWND hwnd_ = nullptr;
        Assets assets_;
        GameState state_ = GameState::Cover;
        Layout layout_{};
        MyPlane player_;
        std::vector<EnemyPlane> enemies_;
        std::vector<Bullet> bullets_;
        std::vector<Explosion> explosions_;
        std::vector<int> historyScores_;
        std::mt19937 rng_;
        SpriteInfo playerSprites_[3]{};
        SpriteInfo playerBullets_[3]{};
        SpriteInfo enemySprite_{};
        SpriteInfo enemyBullets_[2]{};
        SpriteInfo explosionSprite_{};
        int playerSkin_ = 2;
        int bulletSkin_ = 0;
        int score_ = 0;
        int spawnCounter_ = 30;
        int frameTick_ = 0;
        int dyingTicks_ = 0;
        float backgroundOffset_ = 0.0f;
        ButtonId hoverButton_ = ButtonId::None;
        bool playerDeadSaved_ = false;
        std::wstring message_;
    };

    GameApp g_game;
    ULONG_PTR g_gdiplusToken = 0;
}

MyPlane::MyPlane()
    : x_(0.0f), y_(0.0f), width_(72.0f), height_(72.0f), lives_(kPlayerLives), shootCooldown_(0), invincibleTicks_(0), frameTick_(0)
{
}

void MyPlane::Reset(float startX, float startY)
{
    x_ = startX - width_ * 0.5f;
    y_ = startY;
    lives_ = kPlayerLives;
    shootCooldown_ = 0;
    invincibleTicks_ = 0;
    frameTick_ = 0;
}

void MyPlane::Move(float dx, float dy, float minX, float minY, float maxX, float maxY)
{
    x_ = ClampFloat(x_ + dx, minX, maxX);
    y_ = ClampFloat(y_ + dy, minY, maxY);
}

void MyPlane::Tick()
{
    ++frameTick_;
    if (shootCooldown_ > 0)
    {
        --shootCooldown_;
    }
    if (invincibleTicks_ > 0)
    {
        --invincibleTicks_;
    }
}

bool MyPlane::CanShoot() const
{
    return shootCooldown_ == 0;
}

Bullet MyPlane::Shoot()
{
    shootCooldown_ = 8;
    Bullet bullet{};
    bullet.x = x_ + width_ * 0.5f - 7.0f;
    bullet.y = y_ - 10.0f;
    bullet.speedX = 0.0f;
    bullet.speedY = -10.0f;
    bullet.width = 14.0f;
    bullet.height = 28.0f;
    bullet.active = true;
    bullet.fromPlayer = true;
    bullet.spriteIndex = frameTick_ % 3;
    return bullet;
}

void MyPlane::Hit()
{
    if (invincibleTicks_ > 0)
    {
        return;
    }

    if (lives_ > 0)
    {
        --lives_;
    }
    invincibleTicks_ = 18;
}

RECT MyPlane::Bounds() const
{
    return RECT{ static_cast<LONG>(x_), static_cast<LONG>(y_), static_cast<LONG>(x_ + width_), static_cast<LONG>(y_ + height_) };
}

float MyPlane::X() const { return x_; }
float MyPlane::Y() const { return y_; }
float MyPlane::Width() const { return width_; }
float MyPlane::Height() const { return height_; }
int MyPlane::Lives() const { return lives_; }
bool MyPlane::IsInvincible() const { return invincibleTicks_ > 0; }

void MyPlane::SetSize(float width, float height)
{
    width_ = width;
    height_ = height;
}

EnemyPlane::EnemyPlane()
    : x_(0.0f), y_(0.0f), width_(68.0f), height_(68.0f), speedX_(0.0f), speedY_(2.0f), kind_(0), hp_(kEnemyLives), deathTicks_(0), shootCooldown_(0), frameTick_(0)
{
}

void EnemyPlane::Reset(float startX, float startY, int kind, float speedX, float speedY, int hp)
{
    x_ = startX;
    y_ = startY;
    kind_ = kind;
    speedX_ = speedX;
    speedY_ = speedY;
    hp_ = hp;
    deathTicks_ = 0;
    shootCooldown_ = 50 + kind * 18;
    frameTick_ = 0;
}

void EnemyPlane::Move(float dx, float dy)
{
    x_ += dx + speedX_;
    y_ += dy + speedY_;

    if (x_ < 12.0f)
    {
        x_ = 12.0f;
        speedX_ = std::fabs(speedX_);
    }
    if (x_ > static_cast<float>(kClientWidth) - width_ - 12.0f)
    {
        x_ = static_cast<float>(kClientWidth) - width_ - 12.0f;
        speedX_ = -std::fabs(speedX_);
    }
}

void EnemyPlane::Tick()
{
    ++frameTick_;
    if (shootCooldown_ > 0)
    {
        --shootCooldown_;
    }
    if (hp_ <= 0 && deathTicks_ > 0)
    {
        --deathTicks_;
    }
}

bool EnemyPlane::CanShoot() const
{
    return shootCooldown_ == 0;
}

Bullet EnemyPlane::Shoot()
{
    shootCooldown_ = 60 + kind_ * 20;
    Bullet bullet{};
    bullet.x = x_ + width_ * 0.5f - 7.0f;
    bullet.y = y_ + height_ - 2.0f;
    bullet.speedX = speedX_ * 0.3f;
    bullet.speedY = 5.0f + static_cast<float>(kind_) * 0.6f;
    bullet.width = 14.0f;
    bullet.height = 24.0f;
    bullet.active = true;
    bullet.fromPlayer = false;
    bullet.spriteIndex = frameTick_ % 2;
    return bullet;
}

void EnemyPlane::Hit()
{
    if (hp_ > 0)
    {
        --hp_;
    }
}

void EnemyPlane::BeginDeath(int deathTicks)
{
    if (hp_ > 0)
    {
        return;
    }

    deathTicks_ = std::max(deathTicks_, deathTicks);
}

RECT EnemyPlane::Bounds() const
{
    return RECT{ static_cast<LONG>(x_), static_cast<LONG>(y_), static_cast<LONG>(x_ + width_), static_cast<LONG>(y_ + height_) };
}

float EnemyPlane::X() const { return x_; }
float EnemyPlane::Y() const { return y_; }
float EnemyPlane::Width() const { return width_; }
float EnemyPlane::Height() const { return height_; }
int EnemyPlane::Kind() const { return kind_; }
int EnemyPlane::Hp() const { return hp_; }
int EnemyPlane::DeathTicks() const { return deathTicks_; }
bool EnemyPlane::IsDead() const { return hp_ <= 0; }
bool EnemyPlane::IsExpired() const { return hp_ <= 0 && deathTicks_ <= 0; }

void EnemyPlane::SetSize(float width, float height)
{
    width_ = width;
    height_ = height;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        g_game.Init(hwnd);
        SetTimer(hwnd, kTimerId, kFrameMs, nullptr);
        return 0;
    case WM_TIMER:
        if (wParam == kTimerId)
        {
            g_game.Update();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_KEYDOWN:
        g_game.OnKeyDown(wParam);
        return 0;
    case WM_MOUSEMOVE:
        g_game.OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONDOWN:
        g_game.OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        g_game.Render(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, kTimerId);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    GdiplusStartupInput startupInput;
    if (GdiplusStartup(&g_gdiplusToken, &startupInput, nullptr) != Ok)
    {
        return 0;
    }

    const wchar_t className[] = L"AirStrikeGameWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    RECT rect{ 0, 0, kClientWidth, kClientHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    const int windowWidth = rect.right - rect.left;
    const int windowHeight = rect.bottom - rect.top;
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"空战突击 - 飞机射击小游戏",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (screenWidth - windowWidth) / 2,
        (screenHeight - windowHeight) / 2,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (hwnd == nullptr)
    {
        GdiplusShutdown(g_gdiplusToken);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    GdiplusShutdown(g_gdiplusToken);
    return static_cast<int>(msg.wParam);
}
