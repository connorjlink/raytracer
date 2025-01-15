import std;

#include "application.h"

namespace luma
{

}


class RendererLayer : public Walnut::Layer
{
private:
	rt::Renderer renderer;

public:
	RendererLayer()
		: renderer()
	{
	}

public:
	virtual void OnUIRender() override
	{
		{
			ImGui::Begin("Settings");
			ImGui::Text("Last Render: %.3fms", renderer.frametime);
			if (ImGui::Button("Render"))
			{
				renderer.Render();
			}
			ImGui::End();
		}
		
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("Viewport");

			auto avail = ImGui::GetContentRegionAvail();

			renderer.Resize(static_cast<std::uint32_t>(avail.x), 
							static_cast<std::uint32_t>(avail.y));

			if (renderer.image != nullptr)
			{
				ImGui::Image(renderer.image->GetDescriptorSet(),
				{ 
					static_cast<float>(renderer.image->GetWidth()),
					static_cast<float>(renderer.image->GetHeight())
				});
			}

			ImGui::End();
			ImGui::PopStyleVar();

			renderer.Render();
		}
	}
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Raytracer";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<RendererLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});

	return app;
}