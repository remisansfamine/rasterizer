#pragma once

#ifdef SCN_EXPORTS
#define SCN_API __declspec(dllexport)
#else
#define SCN_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    
typedef struct scnImpl scnImpl;
typedef struct rdrImpl rdrImpl;

// Create/Destroy scene
SCN_API scnImpl* scnCreate(void);
SCN_API void scnDestroy(scnImpl* scene);

// Update scene and renders it
SCN_API void scnUpdate(scnImpl* scene, float deltaTime, rdrImpl* renderer);

struct ImGuiContext;
SCN_API void scnSetImGuiContext(scnImpl* scene, struct ImGuiContext* context);
SCN_API void scnShowImGuiControls(scnImpl* scene);

#ifdef __cplusplus
}
#endif