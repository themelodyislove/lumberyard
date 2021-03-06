/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>


namespace AzToolsFramework
{
    namespace Components
    {
        void EditorComponentAPIComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorComponentAPIComponent>();

                serializeContext->RegisterGenericType<AZStd::vector<AZ::EntityComponentIdPair>>();
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<AZ::EntityComponentIdPair>("EntityComponentIdPair")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Components")
                    ->Attribute(AZ::Script::Attributes::Module, "entity")
                    ->Constructor()
                    ->Constructor<AZ::EntityId, AZ::ComponentId>()
                    ->Method("GetEntityId", &AZ::EntityComponentIdPair::GetEntityId)
                    ->Method("GetComponentId", &AZ::EntityComponentIdPair::GetComponentId)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ;

                behaviorContext->EBus<EditorComponentAPIBus>("EditorComponentAPIBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Components")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Event("FindComponentTypeIds", &EditorComponentAPIRequests::FindComponentTypeIds)
                    ->Event("FindComponentTypeNames", &EditorComponentAPIRequests::FindComponentTypeNames)
                    ->Event("BuildComponentTypeNameList", &EditorComponentAPIRequests::BuildComponentTypeNameList)
                    ->Event("AddComponentsOfType", &EditorComponentAPIRequests::AddComponentsOfType)
                    ->Event("HasComponentOfType", &EditorComponentAPIRequests::HasComponentOfType)
                    ->Event("CountComponentsOfType", &EditorComponentAPIRequests::CountComponentsOfType)
                    ->Event("GetComponentOfType", &EditorComponentAPIRequests::GetComponentOfType)
                    ->Event("GetComponentsOfType", &EditorComponentAPIRequests::GetComponentsOfType)
                    ->Event("IsValid", &EditorComponentAPIRequests::IsValid)
                    ->Event("EnableComponents", &EditorComponentAPIRequests::EnableComponents)
                    ->Event("IsComponentEnabled", &EditorComponentAPIRequests::IsComponentEnabled)
                    ->Event("DisableComponents", &EditorComponentAPIRequests::DisableComponents)
                    ->Event("RemoveComponents", &EditorComponentAPIRequests::RemoveComponents)
                    ->Event("BuildComponentPropertyTreeEditor", &EditorComponentAPIRequests::BuildComponentPropertyTreeEditor)
                    ->Event("GetComponentProperty", &EditorComponentAPIRequests::GetComponentProperty)
                    ->Event("SetComponentProperty", &EditorComponentAPIRequests::SetComponentProperty)
                    ->Event("BuildComponentPropertyList", &EditorComponentAPIRequests::BuildComponentPropertyList)
                    ;
            }
        }

        void EditorComponentAPIComponent::Activate()
        {
            EditorComponentAPIBus::Handler::BusConnect();

            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Error("Editor", m_serializeContext, "Serialize context not available");
        }

        void EditorComponentAPIComponent::Deactivate()
        {
            EditorComponentAPIBus::Handler::BusDisconnect();
        }

        AZStd::vector<AZ::Uuid> EditorComponentAPIComponent::FindComponentTypeIds(const AZStd::vector<AZStd::string>& componentTypeNames)
        {
            AZStd::vector<AZ::Uuid> foundTypeIds;

            size_t typesCount = componentTypeNames.size();
            size_t counter = 0;

            foundTypeIds.resize(typesCount);

            m_serializeContext->EnumerateDerived<AZ::Component>(
                [&counter, typesCount, componentTypeNames, &foundTypeIds](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                if (AzToolsFramework::AppearsInGameComponentMenu(*componentClass) && componentClass->m_editData)
                {
                    for (int i = 0; i < typesCount; ++i)
                    {
                        if (componentClass->m_editData->m_name == componentTypeNames[i])
                        {
                            foundTypeIds[i] = componentClass->m_typeId;
                            ++counter;
                        }
                    }

                    if (counter >= typesCount)
                    {
                        return false;
                    }
                }

                return true;
            });

            AZ_Warning("EditorComponentAPI", (counter >= typesCount), "FindComponentTypeIds - Not all Type Names provided could be converted to Type Ids.");
            
            return foundTypeIds;
        }

        AZStd::vector<AZStd::string> EditorComponentAPIComponent::FindComponentTypeNames(const AZ::ComponentTypeList& componentTypeIds)
        {
            AZStd::vector<AZStd::string> foundTypeNames;

            size_t typesCount = componentTypeIds.size();
            size_t counter = 0;

            foundTypeNames.resize(typesCount);

            m_serializeContext->EnumerateDerived<AZ::Component>(
                [&counter, typesCount, componentTypeIds, &foundTypeNames](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                if (AzToolsFramework::AppearsInGameComponentMenu(*componentClass) && componentClass->m_editData)
                {
                    for (int i = 0; i < typesCount; ++i)
                    {
                        if (componentClass->m_typeId == componentTypeIds[i])
                        {
                            foundTypeNames[i] = componentClass->m_editData->m_name;
                            ++counter;
                        }
                    }

                    if (counter >= typesCount)
                    {
                        return false;
                    }
                }

                return true;
            });

            AZ_Warning("EditorComponentAPI", (counter >= typesCount), "FindComponentTypeNames - Not all Type Ids provided could be converted to Type Names.");
            
            return foundTypeNames;
        }

        AZStd::vector<AZStd::string> EditorComponentAPIComponent::BuildComponentTypeNameList()
        {
            AZStd::vector<AZStd::string> typeNameList;

            m_serializeContext->EnumerateDerived<AZ::Component>(
                [&typeNameList](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
            {
                AZ_UNUSED(knownType)

                if (AzToolsFramework::AppearsInGameComponentMenu(*componentClass) && componentClass->m_editData)
                {
                    typeNameList.push_back(componentClass->m_editData->m_name);
                }

                return true;
            });

            return typeNameList;
        }

        // Returns an Outcome object with the Component Id if successful, and the cause of the failure otherwise
        EditorComponentAPIRequests::AddComponentsOutcome EditorComponentAPIComponent::AddComponentsOfType(AZ::EntityId entityId, const AZ::ComponentTypeList& componentTypeIds)
        {
            EditorEntityActionComponent::AddComponentsOutcome outcome;
            EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::AddComponentsToEntities, EntityIdList{ entityId }, componentTypeIds);

            AZ_Warning("EditorComponentAPI", outcome.IsSuccess(), "AddComponentsOfType - AddComponentsToEntities failed (%s).", outcome.GetError().c_str());

            if (!outcome.IsSuccess())
            {
                return AddComponentsOutcome( AZStd::string("AddComponentsOfType - AddComponentsToEntities failed (") + outcome.GetError().c_str() + ")." );
            }
           
            auto entityToComponentMap = outcome.GetValue();

            if (entityToComponentMap.find(entityId) == entityToComponentMap.end() || entityToComponentMap[entityId].m_componentsAdded.size() == 0)
            {
                AZ_Warning("EditorComponentAPI", false, "Malformed result from AddComponentsToEntities.");
                return AddComponentsOutcome( AZStd::string("Malformed result from AddComponentsToEntities.") );
            }
            
            AZStd::vector<AZ::EntityComponentIdPair> componentIds;
            for (AZ::Component* component : entityToComponentMap[entityId].m_componentsAdded)
            {
                if (!component)
                {
                    AZ_Warning("EditorComponentAPI", false, "Invalid component returned in AddComponentsToEntities.");
                    return AddComponentsOutcome( AZStd::string("Invalid component returned in AddComponentsToEntities.") );
                }
                else
                {
                    componentIds.push_back(AZ::EntityComponentIdPair(entityId, component->GetId()));
                }
            }

            return AddComponentsOutcome( componentIds );
        }

        bool EditorComponentAPIComponent::HasComponentOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            GetComponentOutcome outcome = GetComponentOfType(entityId, componentTypeId);
            return outcome.IsSuccess() && outcome.GetValue().GetComponentId() != AZ::InvalidComponentId;
        }

        size_t EditorComponentAPIComponent::CountComponentsOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            AZStd::vector<AZ::Component*> components = FindComponents(entityId, componentTypeId);
            return components.size();
        }

        EditorComponentAPIRequests::GetComponentOutcome EditorComponentAPIComponent::GetComponentOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            AZ::Component* component = FindComponent(entityId, componentTypeId);
            if (component)
            {
                return GetComponentOutcome( AZ::EntityComponentIdPair(entityId, component->GetId()) );
            }
            else
            {
                return GetComponentOutcome( AZStd::string("GetComponentOfType - Component type of id ") + componentTypeId.ToString<AZStd::string>() + " not found on Entity" );
            }
        }

        EditorComponentAPIRequests::GetComponentsOutcome EditorComponentAPIComponent::GetComponentsOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            AZStd::vector<AZ::Component*> components = FindComponents(entityId, componentTypeId);

            if (components.empty())
            {
                return GetComponentsOutcome( AZStd::string("GetComponentOfType - Component type not found on Entity") );
            }

            AZStd::vector<AZ::EntityComponentIdPair> componentIds;
            componentIds.reserve(components.size());

            for (AZ::Component* component : components)
            {
                componentIds.push_back(AZ::EntityComponentIdPair(entityId, component->GetId()));
            }

            return {componentIds};
        }

        bool EditorComponentAPIComponent::IsValid(AZ::EntityComponentIdPair componentInstance)
        {
            AZ::Component* component = FindComponent(componentInstance.GetEntityId() , componentInstance.GetComponentId());
            return component != nullptr;
        }

        bool EditorComponentAPIComponent::EnableComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances)
        {
            AZStd::vector<AZ::Component*> components;
            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
                if (component)
                {
                    components.push_back(component);
                }
                else
                {
                    AZ_Warning("EditorComponentAPI", false, "EnableComponent failed - could not find Component from the given entityId and componentId.");
                    return false;
                }
            }

            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, components);

            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                if (!IsComponentEnabled(componentInstance))
                {
                    return false;
                }
            }
            
            return true;
        }

        bool EditorComponentAPIComponent::IsComponentEnabled(const AZ::EntityComponentIdPair& componentInstance)
        {
            // Get AZ::Entity*
            AZ::Entity* entityPtr = FindEntity(componentInstance.GetEntityId());

            if (!entityPtr)
            {
                AZ_Warning("EditorComponentAPI", false, "IsComponentEnabled failed - could not find Entity from the given entityId");
                return false;
            }

            // Get Component*
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());

            if (!component)
            {
                AZ_Warning("EditorComponentAPI", false, "IsComponentEnabled failed - could not find Component from the given entityId and componentId.");
                return false;
            }

            const auto& entityComponents = entityPtr->GetComponents();
            if (AZStd::find(entityComponents.begin(), entityComponents.end(), component) != entityComponents.end())
            {
                return true;
            }

            return false;
        }

        bool EditorComponentAPIComponent::DisableComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances)
        {
            AZStd::vector<AZ::Component*> components;
            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
                if (component)
                {
                    components.push_back(component);
                }
                else
                {
                    AZ_Warning("EditorComponentAPI", false, "DisableComponent failed - could not find Component from the given entityId and componentId.");
                    return false;
                }
            }

            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::DisableComponents, components);

            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                if (IsComponentEnabled(componentInstance))
                {
                    return false;
                }
            }

            return true;
        }

        bool EditorComponentAPIComponent::RemoveComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances)
        {
            bool cumulativeSuccess = true;

            AZStd::vector<AZ::Component*> components;
            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
                if (component)
                {
                    components.push_back(component);
                }
                else
                {
                    AZ_Warning("EditorComponentAPI", false, "RemoveComponents - a component could not be found.");
                    cumulativeSuccess = false;
                }
            }

            EditorEntityActionComponent::RemoveComponentsOutcome outcome;
            EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::RemoveComponents, components);
            
            if (!outcome.IsSuccess())
            {
                AZ_Warning("EditorComponentAPI", false, "RemoveComponents failed - components could not be removed from entity.");
                return false;
            }

            return cumulativeSuccess;
        }

        EditorComponentAPIRequests::PropertyTreeOutcome EditorComponentAPIComponent::BuildComponentPropertyTreeEditor(const AZ::EntityComponentIdPair& componentInstance)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Warning("EditorComponentAPIComponent", false, "SetComponentProperty - Component Instance is Invalid.");
                return {PropertyTreeOutcome::ErrorType("SetComponentProperty - Component Instance is Invalid.")};
            }

            return {PropertyTreeOutcome::ValueType(reinterpret_cast<void*>(component), component->RTTI_GetType())};
        }

        EditorComponentAPIRequests::PropertyOutcome EditorComponentAPIComponent::GetComponentProperty(const AZ::EntityComponentIdPair& componentInstance, const AZStd::string_view propertyPath)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Warning("EditorComponentAPIComponent", false, "GetComponentProperty - Component Instance is Invalid.");
                return {PropertyOutcome::ErrorType("GetComponentProperty - Component Instance is Invalid.")};
            }

            PropertyTreeEditor pte = PropertyTreeEditor(reinterpret_cast<void*>(component), component->RTTI_GetType());

            return {PropertyOutcome::ValueType(pte.GetProperty(propertyPath))};
        }

        EditorComponentAPIRequests::PropertyOutcome EditorComponentAPIComponent::SetComponentProperty(const AZ::EntityComponentIdPair& componentInstance, const AZStd::string_view propertyPath, const AZStd::any& value)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Warning("EditorComponentAPIComponent", false, "SetComponentProperty - Component Instance is Invalid.");
                return {PropertyOutcome::ErrorType("SetComponentProperty - Component Instance is Invalid.")};
            }

            PropertyTreeEditor pte = PropertyTreeEditor(reinterpret_cast<void*>(component), component->RTTI_GetType());

            PropertyOutcome result = pte.SetProperty(propertyPath, value);
            if (result.IsSuccess())
            {
                PropertyEditorEntityChangeNotificationBus::Event(componentInstance.GetEntityId(), &PropertyEditorEntityChangeNotifications::OnEntityComponentPropertyChanged, componentInstance.GetComponentId());
            }

            return result;
        }

        const AZStd::vector<AZStd::string> EditorComponentAPIComponent::BuildComponentPropertyList(const AZ::EntityComponentIdPair& componentInstance)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Warning("EditorComponentAPIComponent", false, "SetComponentProperty - Component Instance is Invalid.");
                return { AZStd::string("SetComponentProperty - Component Instance is Invalid.") };
            }

            PropertyTreeEditor pte = PropertyTreeEditor(reinterpret_cast<void*>(component), component->RTTI_GetType());

            return pte.BuildPathsList();
        }

        AZ::Entity* EditorComponentAPIComponent::FindEntity(AZ::EntityId entityId)
        {
            AZ_Assert(entityId.IsValid(), "EditorComponentAPIComponent::FindEntity - Invalid EntityId provided.");
            if (!entityId.IsValid())
            {
                return nullptr;
            }

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            return entity;
        }

        AZ::Component* EditorComponentAPIComponent::FindComponent(AZ::EntityId entityId, AZ::ComponentId componentId)
        {
            // Get AZ::Entity*
            AZ::Entity* entityPtr = FindEntity(entityId);

            if (!entityPtr)
            {
                AZ_Warning("EditorComponentAPI", false, "FindComponent failed - could not find entity pointer from entityId provided.");
                return nullptr;
            }

            // See if the component is on the entity proper (Active)
            const auto& entityComponents = entityPtr->GetComponents();
            for (AZ::Component* component : entityComponents)
            {
                if (component->GetId() == componentId)
                {
                    return component;
                }
            }

            // Check for pending components
            AZStd::vector<AZ::Component*> pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entityPtr->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            for (AZ::Component* component : pendingComponents)
            {
                if (component->GetId() == componentId)
                {
                    return component;
                }
            }

            // Check for disabled components
            AZStd::vector<AZ::Component*> disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entityPtr->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            for (AZ::Component* component : disabledComponents)
            {
                if (component->GetId() == componentId)
                {
                    return component;
                }
            }

            return nullptr;
        }

        AZ::Component* EditorComponentAPIComponent::FindComponent(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            // Get AZ::Entity*
            AZ::Entity* entityPtr = FindEntity(entityId);

            if (!entityPtr)
            {
                AZ_Warning("EditorComponentAPI", false, "FindComponent failed - could not find entity pointer from entityId provided.");
                return nullptr;
            }

            // See if the component is on the entity proper (Active)
            const auto& entityComponents = entityPtr->GetComponents();
            for (AZ::Component* component : entityComponents)
            {
                if (component->RTTI_GetType() == componentTypeId)
                {
                    return component;
                }
            }

            // Check for pending components
            AZStd::vector<AZ::Component*> pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entityPtr->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            for (AZ::Component* component : pendingComponents)
            {
                if (component->RTTI_GetType() == componentTypeId)
                {
                    return component;
                }
            }

            // Check for disabled components
            AZStd::vector<AZ::Component*> disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entityPtr->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            for (AZ::Component* component : disabledComponents)
            {
                if (component->RTTI_GetType() == componentTypeId)
                {
                    return component;
                }
            }

            return nullptr;
        }

        AZStd::vector<AZ::Component*> EditorComponentAPIComponent::FindComponents(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            AZStd::vector<AZ::Component*> components;

            // Get AZ::Entity*
            AZ::Entity* entityPtr = FindEntity(entityId);

            if (!entityPtr)
            {
                AZ_Warning("EditorComponentAPI", false, "FindComponents failed - could not find entity pointer from entityId provided.");
                return components;
            }

            // See if the component is on the entity proper (Active)
            const auto& entityComponents = entityPtr->GetComponents();
            for (AZ::Component* component : entityComponents)
            {
                if (component->RTTI_GetType() == componentTypeId)
                {
                    components.push_back(component);
                }
            }

            // Check for pending components
            AZStd::vector<AZ::Component*> pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entityId, &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            for (AZ::Component* component : pendingComponents)
            {
                if (component->RTTI_GetType() == componentTypeId)
                {
                    components.push_back(component);
                }
            }

            // Check for disabled components
            AZStd::vector<AZ::Component*> disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entityId, &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            for (AZ::Component* component : disabledComponents)
            {
                if (component->RTTI_GetType() == componentTypeId)
                {
                    components.push_back(component);
                }
            }

            return components;
        }

    } // Components
} // AzToolsFramework
